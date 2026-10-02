#include "7zip/UI/FileManager/IFolder.h"
#include "Windows/PropVariant.h"
#include "archivenativeopen.h"
#include "qtarchivefileresource.h"

void QtArchiveFileResource::BindNativeItemOrder(IFolderFolder *metadata) {
    _nativeItemOrder = false;
    if (!metadata || !_nativeOpen || !_nativeOpen->hasCommentWriter())
        return;
    IInArchive *native = _nativeOpen->arc().Archive;
    UInt32 nativeCount = 0, count = 0;
    if (native->GetNumberOfItems(&nativeCount) != S_OK ||
        metadata->GetNumberOfItems(&count) != S_OK || nativeCount != count)
        return;
    // The source Agent proxy currently comes from the file-only item transport.
    // Never feed its real indexes to another handler without proving the order.
    for (UInt32 i = 0; i < count; ++i) {
        NWindows::NCOM::CPropVariant original, presented;
        if (native->GetProperty(i, kpidPath, &original) != S_OK ||
            metadata->GetProperty(i, kpidPath, &presented) != S_OK || original.vt != VT_BSTR ||
            presented.vt != VT_BSTR ||
            QString::fromWCharArray(original.bstrVal).normalized(QString::NormalizationForm_C) !=
                QString::fromWCharArray(presented.bstrVal).normalized(QString::NormalizationForm_C))
            return;
    }
    _nativeItemOrder = true;
}

IInArchive *QtArchiveFileResource::NativeItemProperties() const {
    return _nativeItemOrder && _nativeOpen ? _nativeOpen->arc().Archive.Interface() : nullptr;
}

bool QtArchiveFileResource::HasCommentService() const {
    return NativeItemProperties() && _nativeOpen->hasCommentWriter() && HasUpdateService();
}
