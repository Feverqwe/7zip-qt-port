#include "qtarchivedeleteinputs.h"

void QtArchiveDeleteInputs::Capture(const CDirItems &items, const CByteBuffer &processed) {
    DirItems = items;
    DirItems.Callback = nullptr;
    ProcessedItems = processed;
}

void QtArchiveDeleteInputs::MarkCreatedInputsProcessed() {
    ProcessedItems.Alloc(DirItems.Items.Size());
    for (unsigned i = 0; i < DirItems.Items.Size(); ++i)
        ProcessedItems[i] = 1;
}

HRESULT QtArchiveDeleteInputs::Delete(IUpdateDeletionCallback &callback) {
    return UpdateArchive(DirItems, ProcessedItems, SymLinks, &callback);
}

HRESULT QtUpdateDeletionCallback::DeletingAfterArchiving(const FString &path, bool isDir) {
    return Progress ? Progress(path, isDir) : S_OK;
}
HRESULT QtUpdateDeletionCallback::FinishDeletingAfterArchiving() { return S_OK; }
