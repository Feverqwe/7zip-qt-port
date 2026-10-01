#pragma once
#include "CPP/7zip/UI/Common/Update.h"
#include <functional>

// A native enumeration snapshot survives staging and final archive publication.
// It is not a list of paths to delete regardless of subsequent source changes.
class QtArchiveDeleteInputs {
  public:
    CDirItems DirItems;
    CByteBuffer ProcessedItems;
    void Capture(const CDirItems &items, const CByteBuffer &processed);
    void MarkCreatedInputsProcessed();
    HRESULT Delete(IUpdateDeletionCallback &callback);
};

class QtUpdateDeletionCallback final : public IUpdateDeletionCallback {
  public:
    std::function<HRESULT(const FString &, bool)> Progress;
    HRESULT DeletingAfterArchiving(const FString &path, bool isDir) override;
    HRESULT FinishDeletingAfterArchiving() override;
};
