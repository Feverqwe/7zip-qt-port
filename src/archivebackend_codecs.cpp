#include "../third_party/7zip/C/7zTypes.h"
#include "7zip/Common/CreateCoder.h"
#include "7zip/IPassword.h"
#include "7zip/UI/Common/DefaultName.h"
#include "7zip/UI/Common/SetProperties.h"
#include "CPP/7zip/UI/Common/ExtractingFilePath.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "CPP/7zip/UI/Common/Update.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archivenativeopen.h"
#include "qtbenchmarkservices.h"
#include "qtlanguageservices.h"
#include <QFileInfo>
#include <cerrno>

namespace {
HRESULT ValidateNativeUpdatePaths(UInt32 count, IArchiveUpdateCallback *callback,
                                  bool allowAbsolute = false) {
    if (!callback)
        return E_INVALIDARG;
    for (UInt32 i = 0; i < count; ++i) {
        Int32 newData, newProps;
        UInt32 originalIndex;
        RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &originalIndex))
        if (!newProps)
            continue;
        NWindows::NCOM::CPropVariant path;
        RINOK(callback->GetProperty(i, kpidPath, &path))
        // Single-stream handlers may have no stored filename. An unchanged
        // item without new properties preserves its original archive name.
        if (path.vt == VT_EMPTY)
            continue;
        if (path.vt != VT_BSTR)
            return E_INVALIDARG;
        const QString requested = QString::fromWCharArray(path.bstrVal);
        // Original UpdateCallback marks enumerated NTFS streams explicitly.
        // Validate the base path and source-corrected stream name separately;
        // an unmarked colon must never bypass ordinary path validation.
        NWindows::NCOM::CPropVariant stream;
        RINOK(callback->GetProperty(i, kpidIsAltStream, &stream))
        if (stream.vt != VT_EMPTY && stream.vt != VT_BOOL)
            return E_INVALIDARG;
        if (stream.vt != VT_BOOL || stream.boolVal != VARIANT_TRUE) {
            if (!ArchiveBackend::sourceUpdatePathIsSafe(requested, allowAbsolute))
                return E_INVALIDARG;
            continue;
        }
        if (requested.contains(QChar(0)))
            return E_INVALIDARG;
        const int colon = FindAltStreamColon_in_Path(path.bstrVal);
        if (colon <= 0 ||
            !ArchiveBackend::sourceUpdatePathIsSafe(requested.left(colon), allowAbsolute))
            return E_INVALIDARG;
        UString name(path.bstrVal + colon + 1);
        UString corrected = name;
        Correct_AltStream_Name(corrected);
        // The original correction accepts the optional NTFS :$DATA suffix.
        if (name.Len() >= 6 && StringsAreEqualNoCase_Ascii(name.RightPtr(6), ":$DATA"))
            name.DeleteFrom(name.Len() - 6);
        if (name != corrected)
            return E_INVALIDARG;
    }
    return S_OK;
}
} // namespace

void ArchiveBackend::assignDefaultEntryNames(const QString &format) {
    CArc arc;
    arc.Path = archive_.toStdWString().c_str();
    const auto catalog = codecCatalog();
    const int index = catalog && catalog->result() == S_OK ? catalog->formatIndex(format) : -1;
    if (index >= 0)
        arc.SetDefaultName(catalog->nativeCodecs(), index);
    else
        arc.DefaultName = GetDefaultName2(ExtractFileNameFromPath(arc.Path), UString(), UString());
    for (ArchiveEntry &entry : entries_)
        if (entry.path.isEmpty() && !entry.directory)
            entry.path = QString::fromWCharArray(arc.DefaultName.Ptr());
}

ArchiveBackend *ArchiveBackend::createIndependentBackend(QObject *parent) const {
    auto *backend = new ArchiveBackend(parent, configuredEngine_);
    // A new File Manager window owns its process/operation state. Only the
    // immutable module lease is shared, as with existing native readers.
    std::atomic_store(&backend->codecCatalog_, codecCatalog());
    return backend;
}

HRESULT ArchiveBackend::openNativeArchive(const QString &path, const QString &type,
                                          IOpenCallbackUI *callback,
                                          std::shared_ptr<ArchiveNativeOpen> &reader,
                                          const COpenOptions *options) const {
    return openNativeArchive(codecCatalog(), path, type, callback, reader, options);
}

HRESULT ArchiveBackend::openNativeArchive(const std::shared_ptr<const ArchiveCodecCatalog> &catalog,
                                          const QString &path, const QString &type,
                                          IOpenCallbackUI *callback,
                                          std::shared_ptr<ArchiveNativeOpen> &reader,
                                          const COpenOptions *options) const {
    reader.reset();
    // Standalone callers and missing-module discovery retain the existing
    // process-only read fallback. A valid module uses the source opener.
    if (!catalog || catalog->result() != S_OK)
        return S_OK;
    auto opened = std::make_shared<ArchiveNativeOpen>(catalog);
    const HRESULT result = opened->Open(path, type, callback, options);
    if (result == S_OK || result == S_FALSE)
        reader = std::move(opened);
    return result;
}

HRESULT ArchiveBackend::loadCodecCatalog(const QString &module, quint32 engineVersion) {
    if (busy() || codecDiscoveryActive_)
        return E_ABORT;
    if (!engineVersion)
        return E_INVALIDARG;
    // Existing consumers can retain the old immutable catalog/handler lease.
    // A failed replacement publishes no stale or guessed capabilities.
    std::shared_ptr<const ArchiveCodecCatalog> catalog =
        std::make_shared<ArchiveCodecCatalog>(module, engineVersion);
    const HRESULT result = catalog->result();
    process_->setProgram(configuredEngine_);
    std::atomic_store(&codecCatalog_, std::move(catalog));
    return result;
}

HRESULT ArchiveBackend::updateNativeComment(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                            ISequentialOutStream *output, UInt32 count,
                                            IArchiveUpdateCallback *callback) const {
    if (!reader || !reader->hasCommentWriter())
        return E_NOTIMPL;
    CMyComPtr<IOutArchive> writer;
    RINOK(reader->arc().Archive.QueryInterface(IID_IOutArchive, &writer))
    RINOK(ValidateNativeUpdatePaths(count, callback))
    return writer->UpdateItems(output, count, callback);
}

HRESULT ArchiveBackend::updateSourceArchive(const QString &path, const QString &type,
                                            NWildcard::CCensor &censor, CUpdateOptions &options,
                                            CUpdateErrorInfo &errorInfo,
                                            IOpenCallbackUI *openCallback,
                                            IUpdateCallbackUI2 *callback, bool needSetPath) const {
    if (!callback || !openCallback)
        return E_INVALIDARG;
    if (busy())
        return E_ABORT;
    const auto catalog = codecCatalog();
    if (!catalog)
        return E_NOTIMPL;
    return catalog->updateArchive(path, type, censor, options, errorInfo, openCallback, callback,
                                  needSetPath);
}

HRESULT ArchiveBackend::updateSourceArchive(
    const UString &path, const CObjectVector<COpenType> &types, NWildcard::CCensor &censor,
    CUpdateOptions &options, CUpdateErrorInfo &errorInfo, IOpenCallbackUI *openCallback,
    IUpdateCallbackUI2 *callback, bool needSetPath,
    const std::shared_ptr<const ArchiveCodecCatalog> &catalog) const {
    if (!callback || !openCallback || !catalog)
        return E_INVALIDARG;
    // GUI preflight checks busy state before starting this source worker.
    // Worker code only reads the atomically published catalog, never QProcess.
    if (catalog != codecCatalog())
        return E_ABORT;
    RINOK(catalog->updateArchive(path, types, censor, options, errorInfo, openCallback, callback,
                                 needSetPath))
    if (options.SfxMode && !options.StdOutMode) {
        FOR_VECTOR(i, options.Commands) {
            const UString output = options.Commands[i].ArchivePath.GetFinalPath();
            if (!ArchiveSfxModule::MakeExecutable(QString::fromWCharArray(output.Ptr()))) {
#ifdef _WIN32
                errorInfo.SystemError = ERROR_ACCESS_DENIED;
#else
                errorInfo.SystemError = EACCES;
#endif
                errorInfo.Message = QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                                                      "Could not make the SFX archive executable");
                errorInfo.FileNames.Add(us2fs(output));
                return errorInfo.Get_HRESULT_Error();
            }
        }
    }
    return S_OK;
}

HRESULT ArchiveBackend::setNativeArchiveProperties(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                                   const wchar_t *const *names,
                                                   const PROPVARIANT *values, UInt32 count) const {
    if (!reader || !reader->arc().Archive || reader->arc().IsReadOnly || reader->hasNestedStream())
        return E_NOTIMPL;
    CMyComPtr<ISetProperties> properties;
    RINOK(reader->arc().Archive.QueryInterface(IID_ISetProperties, &properties))
    return properties->SetProperties(names, values, count);
}

HRESULT ArchiveBackend::updateNativeArchive(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                            ISequentialOutStream *output, UInt32 count,
                                            IArchiveUpdateCallback *callback) const {
    if (!reader || !reader->arc().Archive || reader->arc().IsReadOnly || reader->hasNestedStream())
        return E_NOTIMPL;
    CMyComPtr<IOutArchive> writer;
    RINOK(reader->arc().Archive.QueryInterface(IID_IOutArchive, &writer))
    RINOK(ValidateNativeUpdatePaths(count, callback))
    // AgentOut::CommonUpdate retains the handler's properties and calls it
    // directly with the original callback, including tree/raw/crypto services.
    return writer->UpdateItems(output, count, callback);
}

HRESULT ArchiveBackend::updateNativeArchive(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                            ISequentialOutStream *output, UInt32 count,
                                            IArchiveUpdateCallback *callback,
                                            const CompressionOptions &options) const {
    if (!reader || !reader->arc().Archive || reader->arc().IsReadOnly || reader->hasNestedStream())
        return E_NOTIMPL;
    CMyComPtr<IOutArchive> writer;
    RINOK(reader->arc().Archive.QueryInterface(IID_IOutArchive, &writer))
    RINOK(ValidateNativeUpdatePaths(count, callback, options.PathMode == NWildcard::k_AbsPath))
    const auto *format = reader->formatInfo();
    if (!format || (options.SymLinks.Val && !format->Flags_SymLinks()) ||
        (options.HardLinks.Val && !format->Flags_HardLinks()) ||
        (options.AltStreams.Val && !format->Flags_AltStreams()) ||
        (options.NtSecurity.Val && !format->Flags_NtSecurity()))
        return E_NOTIMPL;
    CMyComPtr<ICryptoGetTextPassword2> crypto;
    callback->QueryInterface(IID_ICryptoGetTextPassword2, (void **)&crypto);
    QString password;
    if (crypto) {
        Int32 defined = 0;
        CMyComBSTR value;
        RINOK(crypto->CryptoGetTextPassword2(&defined, &value))
        if (defined)
            password = QString::fromWCharArray(value);
    }
    CObjectVector<CProperty> properties;
    QString error;
    if (!compressionProperties(QString::fromWCharArray(format->Name.Ptr()).toLower(), password,
                               options.EncryptHeaders, options, &properties, &error))
        return E_INVALIDARG;
    // Common/Update.cpp's writer property and callback sequence. No staging
    // layer intervenes between native reparse properties and the handler.
    RINOK(SetProperties(writer, properties))
    return writer->UpdateItems(output, count, callback);
}

HRESULT ArchiveBackend::extractNativeArchive(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                             const UInt32 *indices, UInt32 count, Int32 testMode,
                                             IArchiveExtractCallback *callback) const {
    if (!reader || !reader->arc().Archive)
        return E_NOTIMPL;
    if (reader->hasNativeHashExtractor()) {
        RINOK(validateNativeHashReads(*reader, indices, count))
    }
    return reader->arc().Archive->Extract(indices, count, testMode, callback);
}

bool ArchiveBackend::discoverCodecCatalog(const QString &module) {
    if (busy())
        return false;
    try {
        if (!codecProbe_)
            codecProbe_ = std::make_unique<ArchiveCodecProbe>();
        const bool started =
            codecProbe_->Start(configuredEngine_, module, [this](ArchiveCodecProbeResult result) {
                // Keep UI receivers outside the probe's completion frame so
                // closing the backend cannot invalidate that delivery's owner.
                QMetaObject::invokeMethod(
                    this,
                    [this, result = std::move(result)] {
                        codecDiscoveryActive_ = false;
                        process_->setProgram(!codecDiscoveryCancelled_ && result.result == S_OK &&
                                                     !result.catalog->runtimeEngine().isEmpty()
                                                 ? result.catalog->runtimeEngine()
                                                 : configuredEngine_);
                        std::atomic_store(&codecCatalog_,
                                          codecDiscoveryCancelled_
                                              ? std::shared_ptr<const ArchiveCodecCatalog>()
                                              : result.catalog);
                        emit codecCatalogReady(
                            !codecDiscoveryCancelled_ && result.result == S_OK,
                            codecDiscoveryCancelled_
                                ? QCoreApplication::translate("archivebackend_codecs",
                                                              "Codec discovery cancelled.")
                                : result.error);
                    },
                    Qt::QueuedConnection);
            });
        if (started) {
            codecDiscoveryActive_ = true;
            codecDiscoveryCancelled_ = false;
        }
        return started;
    } catch (...) {
        return false;
    }
}

HRESULT ArchiveBackend::loadHashProviders(CExternalCodecs &providers) const {
    providers.ClearAndRelease();
    const auto catalog = codecCatalog();
    if (!catalog || catalog->result() != S_OK)
        return S_OK; // Original registered hashers remain available without a DLL.
    auto &codecs = const_cast<CCodecs &>(catalog->nativeCodecs());
    providers.GetHashers = static_cast<IHashers *>(&codecs);
    return providers.Load();
}

HRESULT ArchiveBackend::loadBenchmarkProviders(CExternalCodecs &providers) const {
    providers.ClearAndRelease();
    const auto catalog = codecCatalog();
    if (!catalog || catalog->result() != S_OK)
        return E_NOTIMPL;
    auto &codecs = const_cast<CCodecs &>(catalog->nativeCodecs());
    providers.GetCodecs = static_cast<ICompressCodecsInfo *>(&codecs);
    providers.GetHashers = static_cast<IHashers *>(&codecs);
    return providers.Load();
}

std::unique_ptr<IQtBenchmarkServices> ArchiveBackend::benchmarkServices() const {
    return std::make_unique<QtBenchmarkServices>();
}

void ArchiveBackend::cancelCodecDiscovery() {
    if (codecDiscoveryActive_)
        codecDiscoveryCancelled_ = true;
    if (codecProbe_)
        codecProbe_->Cancel();
}

QString ArchiveBackend::openDialogFilter() const {
    const QString allFiles =
        QCoreApplication::translate("archivebackend_codecs", "All files") + " (*)";
    const auto catalog = codecCatalog();
    if (!catalog || catalog->result() != S_OK)
        return allFiles;
    QStringList masks;
    for (const QString &extension : catalog->extensions())
        masks.append("*." + extension);
    if (masks.isEmpty())
        return allFiles;
    return QCoreApplication::translate("archivebackend_codecs", "Archives") + " (" +
           masks.join(' ') + ");;" + allFiles;
}

bool ArchiveBackend::supportsArchiveUpdate(const QString &format) const {
    // Callers supply the detected native type. Standalone compatibility has
    // only its previous ZIP/7z services; catalog consumers use source flags.
    const auto catalog = codecCatalog();
    return catalog ? catalog->result() == S_OK && catalog->canUpdate(format)
                   : !format.compare("zip", Qt::CaseInsensitive) ||
                         !format.compare("7z", Qt::CaseInsensitive);
}

ArchiveSfxModule ArchiveBackend::sfxModule(const QString &path) const {
    const auto catalog = codecCatalog();
    return catalog && catalog->result() == S_OK ? catalog->sfxModule(path)
                                                : ArchiveSfxModule::Read({});
}

bool ArchiveBackend::selectCreationFormat(const QString &archive, const QStringList &sources,
                                          const CompressionOptions &options, QString &format,
                                          QString &error) const {
    const auto catalog = codecCatalog();
    if (!catalog) {
        // Existing standalone callers retain their implemented typed transports.
        format = options.format.isEmpty() ? QFileInfo(archive).suffix().toLower()
                                          : options.format.toLower();
        if ((format == "7z" || format == "zip")
#ifdef Q_OS_WIN
            // These handlers do not advertise source GUI link storage. The
            // standalone engine loses directory reparse metadata; require a
            // catalog-backed handler before allowing Windows link storage.
            && !options.SymLinks.Val
#endif
            && !options.HardLinks.Val && !options.AltStreams.Val && !options.NtSecurity.Val)
            return true;
    } else if (catalog->result() != S_OK) {
        error = catalog->error();
        return false;
    } else {
        const auto &codecs = catalog->nativeCodecs();
        const int index =
            options.format.isEmpty()
                ? codecs.FindFormatForArchiveName(UString(archive.toStdWString().c_str()))
                : catalog->formatIndex(options.format);
        if (index >= 0 && !codecs.Formats[unsigned(index)].Flags_HashHandler() &&
            (!options.SymLinks.Val || codecs.Formats[unsigned(index)].Flags_SymLinks()) &&
            (!options.HardLinks.Val || codecs.Formats[unsigned(index)].Flags_HardLinks()) &&
            (!options.AltStreams.Val || codecs.Formats[unsigned(index)].Flags_AltStreams()) &&
            (!options.NtSecurity.Val || codecs.Formats[unsigned(index)].Flags_NtSecurity())) {
            CObjectVector<NWildcard::CCensorPath> censor;
            for (const QString &path : sources)
                censor.AddNew().Path = path.toStdWString().c_str();
            CCompressDialog selection;
            selection.Info.FormatIndex = index;
            if (NCompressDialog::PrepareFormatSelection(selection, codecs, censor) == S_OK &&
                selection.ArcIndices.FindInSorted(unsigned(index)) >= 0) {
                format =
                    QString::fromWCharArray(codecs.Formats[unsigned(index)].Name.Ptr()).toLower();
                return true;
            }
        }
    }
    error = QCoreApplication::translate(
        "archivebackend_codecs",
        "No archive creation engine is available for this format and selection.");
    return false;
}
