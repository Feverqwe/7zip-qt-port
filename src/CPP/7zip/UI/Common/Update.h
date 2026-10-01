// Update.h
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/Common/DirItem.h"
#include "7zip/UI/Common/UpdateAction.h"
#include "7zip/UI/Common/UpdateProduce.h"

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
