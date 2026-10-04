#include "archivenativeopen.h"
#include "archivecodeccatalog.h"
#include <QScopeGuard>

ArchiveNativeOpen::~ArchiveNativeOpen() { Close(); }

HRESULT ArchiveNativeOpen::Open(const QString &path, const QString &type, IOpenCallbackUI *callback,
                                const COpenOptions *requested) {
    callback_ = callback;
    // Preserve the empty source type's recursive intent explicitly at the
    // file-only listing/extract/reopen integration boundary.
    transportType_ = type.isEmpty() ? QStringLiteral("*:r") : type;
    const auto detach = qScopeGuard([&] { callback_ = nullptr; });
    // Volume streams can retain the source COpenCallbackImp beyond opening.
    // Its UI pointer must remain valid, without retaining the opening dialog.
    if (requested) {
        COpenOptions options = *requested;
        options.filePath = path.toStdWString().c_str();
        return catalog_->openArchive(link_, options, this);
    }
    return catalog_->openArchive(link_, path, type, this);
}

HRESULT ArchiveNativeOpen::Open_CheckBreak() {
    return callback_ ? callback_->Open_CheckBreak() : S_OK;
}
HRESULT ArchiveNativeOpen::Open_SetTotal(const UInt64 *files, const UInt64 *bytes) {
    return callback_ ? callback_->Open_SetTotal(files, bytes) : S_OK;
}
HRESULT ArchiveNativeOpen::Open_SetCompleted(const UInt64 *files, const UInt64 *bytes) {
    return callback_ ? callback_->Open_SetCompleted(files, bytes) : S_OK;
}
HRESULT ArchiveNativeOpen::Open_Finished() { return callback_ ? callback_->Open_Finished() : S_OK; }
HRESULT ArchiveNativeOpen::Open_CryptoGetTextPassword(BSTR *password) {
    return callback_ ? callback_->Open_CryptoGetTextPassword(password) : E_ABORT;
}

HRESULT ArchiveNativeOpen::Close() {
    const HRESULT result = link_.IsOpen ? link_.Close() : S_OK;
    link_.Release();
    return result;
}

bool ArchiveNativeOpen::hasCommentWriter() const {
    if (!link_.IsOpen || link_.Arcs.Size() != 1 || arc().IsReadOnly || arc().ArcStreamOffset != 0)
        return false;
    const auto &formats = catalog_->nativeCodecs().Formats;
    // The source parser owner uses FormatIndex=-1 rather than a codec slot.
    if (arc().FormatIndex < 0 || unsigned(arc().FormatIndex) >= formats.Size())
        return false;
    const auto &format = formats[unsigned(arc().FormatIndex)];
    return format.Is_Zip() && format.LibIndex < 0;
}

const CArcInfoEx *ArchiveNativeOpen::formatInfo() const {
    if (!link_.IsOpen)
        return nullptr;
    const auto &formats = catalog_->nativeCodecs().Formats;
    const int index = arc().FormatIndex;
    return index >= 0 && unsigned(index) < formats.Size() ? &formats[unsigned(index)] : nullptr;
}

bool ArchiveNativeOpen::hasNativeRarExtractor() const {
    if (!link_.IsOpen)
        return false;
    const auto &formats = catalog_->nativeCodecs().Formats;
    if (arc().FormatIndex < 0 || unsigned(arc().FormatIndex) >= formats.Size())
        return false;
    const auto &format = formats[unsigned(arc().FormatIndex)];
    return format.Name.IsEqualTo("Rar5") && format.LibIndex < 0;
}

bool ArchiveNativeOpen::hasNativeHashExtractor() const {
    if (!link_.IsOpen)
        return false;
    const auto &formats = catalog_->nativeCodecs().Formats;
    if (arc().FormatIndex < 0 || unsigned(arc().FormatIndex) >= formats.Size())
        return false;
    const auto &format = formats[unsigned(arc().FormatIndex)];
    return format.Flags_HashHandler() && format.LibIndex < 0;
}
