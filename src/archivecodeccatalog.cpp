#include "archivecodeccatalog.h"
#include "7zip/Archive/Zip/ZipHandler.h"
#include "Common/StringConvert.h"
#include "qtlanguageservices.h"
#include <QDir>

namespace {
IInArchive *CreateSourceZipReader() { return new NArchive::NZip::CHandler; }
IOutArchive *CreateSourceZipWriter() { return new NArchive::NZip::CHandler; }
} // namespace

ArchiveCodecCatalog::ArchiveCodecCatalog(const QString &module, UInt32 expectedVersion) {
    codecs_ = new CCodecs;
    owner_ = codecs_;
    bool loaded = false;
    const auto path = QDir::toNativeSeparators(module).toStdWString();
    result_ = codecs_->LoadDll(us2fs(path.c_str()), false, &loaded);
    UString message;
    codecs_->GetCodecsErrorMessage(message);
    if (result_ != S_OK || !loaded || !codecs_->Errors.IsEmpty() || codecs_->Libs.IsEmpty() ||
        codecs_->Formats.IsEmpty()) {
        result_ = E_FAIL;
        error_ = message.IsEmpty() ? QtLocalText("The official 7-Zip codec module is unavailable.",
                                                 "Официальный модуль форматов 7-Zip недоступен.")
                                   : QString::fromWCharArray(message.Ptr());
    } else {
        version_ = codecs_->Libs[0].Version;
        if (!version_ || (expectedVersion && version_ != expectedVersion)) {
            result_ = E_FAIL;
            error_ = QtLocalText("7zz and the codec module have different versions.",
                                 "Версии 7zz и модуля форматов различаются.");
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
                format.LibIndex = -1;
                format.CreateInArchive = CreateSourceZipReader;
                format.CreateOutArchive = CreateSourceZipWriter;
            }
        }
        codecs_->Formats.Sort();
    }
}

ArchiveCodecCatalog::~ArchiveCodecCatalog() {
    codecs_->Formats.Clear();
    codecs_->CloseLibs();
}

int ArchiveCodecCatalog::formatIndex(const QString &type) const {
    return result_ == S_OK ? codecs_->FindFormatForArchiveType(UString(type.toStdWString().c_str()))
                           : -1;
}

bool ArchiveCodecCatalog::canUpdate(const QString &type) const {
    const int index = formatIndex(type);
    return index >= 0 && codecs_->Formats[unsigned(index)].UpdateEnabled;
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
    return link.Open3(options, callback);
}
