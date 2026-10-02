#include "archivenativeopen.h"
#include "archivecodeccatalog.h"
#include <QScopeGuard>

ArchiveNativeOpen::~ArchiveNativeOpen() { Close(); }

HRESULT ArchiveNativeOpen::Open(const QString &path, const QString &type,
                                IOpenCallbackUI *callback) {
    callback_ = callback;
    // Preserve the empty source type's recursive intent explicitly at the
    // file-only listing/extract/reopen integration boundary.
    transportType_ = type.isEmpty() ? QStringLiteral("*:r") : type;
    const auto detach = qScopeGuard([&] { callback_ = nullptr; });
    // Volume streams can retain the source COpenCallbackImp beyond opening.
    // Its UI pointer must remain valid, without retaining the opening dialog.
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
