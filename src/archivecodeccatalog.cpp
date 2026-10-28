#include "archivecodeccatalog.h"
#include "7zip/Archive/Rar/Rar5Handler.h"
#include "7zip/Archive/Zip/ZipHandler.h"
#include "CPP/7zip/UI/Common/HashCalc.h"
#include "CPP/7zip/UI/Common/Update.h"
#include "Common/StringConvert.h"
#include "archivecodecimage.h"
#include "qtlanguageservices.h"
#include <QDir>
#include <QFileInfo>
#include <QSysInfo>

namespace {
IInArchive *CreateSourceZipReader() { return new NArchive::NZip::CHandler; }
IOutArchive *CreateSourceZipWriter() { return new NArchive::NZip::CHandler; }
IInArchive *CreateSourceRarReader() { return new NArchive::NRar5::CHandler; }
} // namespace

ArchiveCodecCatalog::ArchiveCodecCatalog(const QString &module, UInt32 expectedVersion,
                                         Scope scope) {
    codecs_ = new CCodecs;
    owner_ = codecs_;
    bool loaded = false;
    QString nativeModule = module;
    const bool deployment = scope == Scope::Deployment;
    if (deployment) {
        auto image = ArchiveCodecImage::Create(module);
        if (!image) {
            error_ = QCoreApplication::translate("archivecodeccatalog",
                                                 "Could not prepare the codec runtime snapshot.");
            return;
        }
        image_ = std::move(*image);
        nativeModule = image_->module();
    }
    const auto path = QDir::toNativeSeparators(nativeModule).toStdWString();
    result_ = codecs_->LoadDll(us2fs(path.c_str()), false, &loaded);
    if (result_ == S_OK && deployment) {
        if (!loaded)
            codecs_->MainDll_ErrorPath = us2fs(module.toStdWString().c_str());
        result_ = loadDeploymentPlugins();
    }
    UString message;
    codecs_->GetCodecsErrorMessage(message);
    if (result_ != S_OK || (!deployment && (!loaded || !codecs_->Errors.IsEmpty())) ||
        codecs_->Libs.IsEmpty() || codecs_->Formats.IsEmpty()) {
        result_ = E_FAIL;
        if (error_.isEmpty())
            error_ =
                message.IsEmpty()
                    ? QCoreApplication::translate("archivecodeccatalog",
                                                  "The official 7-Zip codec module is unavailable.")
                    : QString::fromWCharArray(message.Ptr());
    } else {
        version_ = codecs_->Libs[0].Version;
        if (!version_ || (expectedVersion && version_ != expectedVersion)) {
            result_ = E_FAIL;
            error_ = QCoreApplication::translate("archivecodeccatalog",
                                                 "The codec module has an unexpected version.");
        }
    }
    if (result_ != S_OK) {
        codecs_->Formats.Clear();
        codecs_->CloseLibs();
    } else {
        // Keep verified module metadata/indexes for source detection, but use
        // our copied ZIP handler for the in-process property update consumer.
        // OpenArchive still initializes its per-instance external codec lease.
        FOR_VECTOR(i, codecs_->Formats) {
            auto &format = codecs_->Formats[i];
            if (format.Is_Zip()) {
                // These source factories are hooked after Load's g_NumArcs
                // decision; retain their per-instance external codec context.
                codecs_->NeedSetLibCodecs = true;
                format.LibIndex = -1;
                format.CreateInArchive = CreateSourceZipReader;
                format.CreateOutArchive = CreateSourceZipWriter;
            } else if (format.Name.IsEqualTo("Rar5")) {
                codecs_->NeedSetLibCodecs = true;
                format.LibIndex = -1;
                format.CreateInArchive = CreateSourceRarReader;
            }
        }
        Codecs_AddHashArcHandler(codecs_);
        codecs_->NeedSetLibCodecs = true;
        codecs_->Formats.Sort();
    }
}

ArchiveCodecCatalog::~ArchiveCodecCatalog() {
    codecs_->Formats.Clear();
    codecs_->CloseLibs();
}

HRESULT ArchiveCodecCatalog::loadDeploymentPlugins() {
    const QString baseFolder = image_->baseFolder();
    RINOK(codecs_->LoadExternalModules(us2fs(baseFolder.toStdWString().c_str())))
    FOR_VECTOR(i, codecs_->Libs) {
        auto &library = codecs_->Libs[i];
        const QString original = image_->originalPath(QString::fromWCharArray(fs2us(library.Path)));
        library.Path = us2fs(original.toStdWString().c_str());
    }
    FOR_VECTOR(i, codecs_->Errors) {
        auto &error = codecs_->Errors[i];
        const QString original = image_->originalPath(QString::fromWCharArray(fs2us(error.Path)));
        error.Path = us2fs(original.toStdWString().c_str());
    }
    UString message;
    codecs_->GetCodecsErrorMessage(message);
    error_ = QString::fromWCharArray(message.Ptr());
    return codecs_->SetLibCodecs();
}

int ArchiveCodecCatalog::formatIndex(const QString &type) const {
    return result_ == S_OK ? codecs_->FindFormatForArchiveType(UString(type.toStdWString().c_str()))
                           : -1;
}

bool ArchiveCodecCatalog::canUpdate(const QString &type) const {
    const int index = formatIndex(type);
    return index >= 0 && codecs_->Formats[unsigned(index)].UpdateEnabled;
}

ArchiveSfxModule ArchiveCodecCatalog::sfxModule(const QString &path) const {
    if (result_ != S_OK)
        return {};
    auto module = path.isEmpty() ? ArchiveSfxModule::Discover(version_)
                                 : ArchiveSfxModule::Read(path, version_);
    if (!module.available())
        return module;
#ifdef Q_OS_WIN
    const QString type = "PE";
#elif defined(Q_OS_MACOS)
    const QString type = "MachO";
#else
    const QString type = "ELF";
#endif
    // The original executable handler validates structure/CPU. Do not invent
    // another platform executable parser at this integration boundary.
    CArchiveLink link;
    if (openArchive(link, module.path(), type, nullptr) != S_OK || link.Arcs.IsEmpty())
        return ArchiveSfxModule::Read({});
    NWindows::NCOM::CPropVariant cpu;
    if (link.GetArc()->Archive->GetArchiveProperty(kpidCpu, &cpu) != S_OK || cpu.vt != VT_BSTR)
        return ArchiveSfxModule::Read({});
    QString actual = QString::fromWCharArray(cpu.bstrVal).toLower();
    actual.replace('-', '_');
    if (actual == "aarch64")
        actual = "arm64";
    if (actual == "amd64" || actual == "x64")
        actual = "x86_64";
    QString host = QSysInfo::buildCpuArchitecture().toLower();
    if (host == "i386")
        host = "x86";
    if (actual != host)
        return ArchiveSfxModule::Read({});
    return module;
}

QStringList ArchiveCodecCatalog::extensions() const {
    QStringList result;
    if (result_ != S_OK)
        return result;
    // Presentation projection only: source CArcInfoEx owns aliases and AddExt.
    FOR_VECTOR(i, codecs_->Formats) {
        const auto &format = codecs_->Formats[i];
        FOR_VECTOR(j, format.Exts) {
            const QString ext = QString::fromWCharArray(format.Exts[j].Ext.Ptr());
            if (!ext.isEmpty() && !result.contains(ext, Qt::CaseInsensitive))
                result.append(ext);
        }
    }
    return result;
}

void ArchiveCodecCatalog::getUserCodecs(CObjectVector<CCodecInfoUser> &codecs) const {
    codecs_->Get_CodecsInfoUser_Vector(codecs);
}

bool ArchiveCodecCatalog::parseOpenTypes(const QString &types,
                                         CObjectVector<COpenType> &parsed) const {
    parsed.Clear();
    return result_ == S_OK &&
           ParseOpenTypes(*codecs_, UString(types.toStdWString().c_str()), parsed);
}

HRESULT ArchiveCodecCatalog::openArchive(CArchiveLink &link, const QString &path,
                                         const QString &type, IOpenCallbackUI *callback) const {
    if (result_ != S_OK)
        return result_;
    CObjectVector<COpenType> types;
    if (!parseOpenTypes(type, types))
        return E_INVALIDARG;
    CIntVector excluded;
    COpenOptions options;
    // The source API accepts CCodecs*, but only reads its catalog/factories.
    // Keep that pointer confined to this owner rather than exposing mutable metadata.
    options.codecs = codecs_;
    options.types = &types;
    options.excludedFormats = &excluded;
    options.props = nullptr;
    options.filePath = path.toStdWString().c_str();
    return openArchive(link, options, callback);
}

HRESULT ArchiveCodecCatalog::openArchive(CArchiveLink &link, const COpenOptions &requested,
                                         IOpenCallbackUI *callback) const {
    if (result_ != S_OK)
        return result_;
    COpenOptions options = requested;
    // The source type indexes refer to this retained catalog or its copied
    // metadata table. Factories always come from the lifetime owner here.
    options.codecs = codecs_;
    options.QtOpenService = nullptr;
    return link.Open3(options, callback);
}

HRESULT ArchiveCodecCatalog::updateArchive(const QString &path, const QString &type,
                                           NWildcard::CCensor &censor, CUpdateOptions &options,
                                           CUpdateErrorInfo &errorInfo,
                                           IOpenCallbackUI *openCallback,
                                           IUpdateCallbackUI2 *callback, bool needSetPath) const {
    if (result_ != S_OK)
        return result_;
    CObjectVector<COpenType> types;
    if (!parseOpenTypes(type, types))
        return E_INVALIDARG;
    return updateArchive(UString(path.toStdWString().c_str()), types, censor, options, errorInfo,
                         openCallback, callback, needSetPath);
}

HRESULT ArchiveCodecCatalog::updateArchive(const UString &path,
                                           const CObjectVector<COpenType> &types,
                                           NWildcard::CCensor &censor, CUpdateOptions &options,
                                           CUpdateErrorInfo &errorInfo,
                                           IOpenCallbackUI *openCallback,
                                           IUpdateCallbackUI2 *callback, bool needSetPath) const {
    if (result_ != S_OK)
        return result_;
    // The source API reads factories through CCodecs*. Keep the pointer in its
    // lifetime owner, as for OpenArchive; copied format tables cannot own DLLs.
    return ::UpdateArchive(codecs_, types, path, censor, options, errorInfo, openCallback, callback,
                           needSetPath);
}
