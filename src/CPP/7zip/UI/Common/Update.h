// Update.h
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/Common/DirItem.h"
#include "7zip/UI/Common/UpdateAction.h"
#include "7zip/UI/Common/UpdateProduce.h"

struct CUpdateErrorInfo {
    DWORD SystemError;
    AString Message;
    FStringVector FileNames;
    bool ThereIsError() const {
        return SystemError != 0 || !Message.IsEmpty() || !FileNames.IsEmpty();
    }
    HRESULT Get_HRESULT_Error() const {
        return SystemError == 0 ? E_FAIL : HRESULT_FROM_WIN32(SystemError);
    }
    void SetFromLastError(const char *message);
    HRESULT SetFromLastError(const char *message, const FString &fileName);
    HRESULT SetFromError_DWORD(const char *message, const FString &fileName, DWORD error);
    CUpdateErrorInfo() : SystemError(0) {}
};

// Original UpdateArchive SFX-module validation. Qt supplies the runtime folder
// instead of the Windows executable/registry installation directory.
HRESULT PrepareSfxModule(FString &module, const FString &baseFolder, CUpdateErrorInfo &errorInfo);
// Compress's separate SFX-module output for volume creation. The Qt worker
// supplies a staging path; the original streams and copy body remain here.
HRESULT WriteSfxModule(const FString &module, const FString &realPath, CUpdateErrorInfo &errorInfo);

// Narrow publication adapter for UpdateArchive's deletion phase. The archive
// writer is the official external engine; its transaction must finish first.
class IUpdateDeletionCallback {
  public:
    virtual ~IUpdateDeletionCallback() = default;
    virtual HRESULT DeletingAfterArchiving(const FString &path, bool isDir) = 0;
    virtual HRESULT FinishDeletingAfterArchiving() = 0;
};

HRESULT CheckDeleteAfterCompressingActionSet(const NUpdateArchive::CActionSet &actionSet);
void GetUpdateArchiveMTime(const CDirItems &dirItems, const CObjectVector<CArcItem> &arcItems,
                           const CRecordVector<CUpdatePair2> &updatePairs2, CFiTime &ft,
                           bool &ft_Defined);
HRESULT UpdateArchive(const CDirItems &dirItems, const CByteBuffer &processedItems,
                      const CBoolPair &SymLinks, IUpdateDeletionCallback *callback);
