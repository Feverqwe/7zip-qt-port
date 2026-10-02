#include "7zip/Common/CreateCoder.h"
#include "7zip/UI/Common/DefaultName.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archivenativeopen.h"
#include "qtlanguageservices.h"
#include <QFileInfo>

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

HRESULT ArchiveBackend::openNativeArchive(const QString &path, const QString &type,
                                          IOpenCallbackUI *callback,
                                          std::shared_ptr<ArchiveNativeOpen> &reader) const {
    reader.reset();
    const auto catalog = codecCatalog();
    // Standalone callers and missing-module discovery retain the existing
    // process-only read fallback. A valid module uses the source opener.
    if (!catalog || catalog->result() != S_OK)
        return S_OK;
    auto opened = std::make_shared<ArchiveNativeOpen>(catalog);
    const HRESULT result = opened->Open(path, type, callback);
    if (result == S_OK)
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
    process_.setProgram(configuredEngine_);
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
    return writer->UpdateItems(output, count, callback);
}

HRESULT ArchiveBackend::extractNativeArchive(const std::shared_ptr<ArchiveNativeOpen> &reader,
                                             const UInt32 *indices, UInt32 count, Int32 testMode,
                                             IArchiveExtractCallback *callback) const {
    if (!reader || (!reader->hasNativeRarExtractor() && !reader->hasNativeHashExtractor()))
        return E_NOTIMPL;
    if (reader->hasNativeHashExtractor()) {
        RINOK(validateNativeHashReads(*reader, indices, count))
    }
    return reader->arc().Archive->Extract(indices, count, testMode, callback);
}

bool ArchiveBackend::discoverCodecCatalog(const QString &module) {
    if (busy())
        return false;
    if (!codecProbe_)
        codecProbe_ = std::make_unique<ArchiveCodecProbe>();
    codecDiscoveryActive_ = true;
    codecDiscoveryCancelled_ = false;
    return codecProbe_->Start(configuredEngine_, module, [this](ArchiveCodecProbeResult result) {
        // Delivery is queued so a UI receiver can close/delete the backend
        // without deleting the probe's QProcess during its finished callback.
        QMetaObject::invokeMethod(
            this,
            [this, result = std::move(result)] {
                codecDiscoveryActive_ = false;
                process_.setProgram(!codecDiscoveryCancelled_ && result.result == S_OK &&
                                            !result.catalog->runtimeEngine().isEmpty()
                                        ? result.catalog->runtimeEngine()
                                        : configuredEngine_);
                std::atomic_store(&codecCatalog_, codecDiscoveryCancelled_
                                                      ? std::shared_ptr<const ArchiveCodecCatalog>()
                                                      : result.catalog);
                emit codecCatalogReady(!codecDiscoveryCancelled_ && result.result == S_OK,
                                       codecDiscoveryCancelled_
                                           ? QtLocalText("Codec discovery cancelled.",
                                                         "Поиск модуля форматов отменён.")
                                           : result.error);
            },
            Qt::QueuedConnection);
    });
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

void ArchiveBackend::cancelCodecDiscovery() {
    if (codecDiscoveryActive_)
        codecDiscoveryCancelled_ = true;
    if (codecProbe_)
        codecProbe_->Cancel();
}

QString ArchiveBackend::openDialogFilter() const {
    const QString allFiles = QtLocalText("All files", "Все файлы") + " (*)";
    const auto catalog = codecCatalog();
    if (!catalog || catalog->result() != S_OK)
        return allFiles;
    QStringList masks;
    for (const QString &extension : catalog->extensions())
        masks.append("*." + extension);
    if (masks.isEmpty())
        return allFiles;
    return QtLocalText("Archives", "Архивы") + " (" + masks.join(' ') + ");;" + allFiles;
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
        if (format == "7z" || format == "zip")
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
        if (index >= 0 && !codecs.Formats[unsigned(index)].Flags_HashHandler()) {
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
    error = QtLocalText("No archive creation engine is available for this format and selection.",
                        "Для этого формата и выбранных источников нет модуля создания архива.");
    return false;
}
