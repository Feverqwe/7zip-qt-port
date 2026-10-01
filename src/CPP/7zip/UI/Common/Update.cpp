// Update.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// UpdateArchive's action-set/deletion phases and Compress's latest-time loop only.
#include "Update.h"
#include "../../../Windows/FileDir.h"
#include "OpenArchive.h"
#include "Windows/TimeUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;

HRESULT CheckDeleteAfterCompressingActionSet(const NUpdateArchive::CActionSet &as) {
    using namespace NUpdateArchive;
    for (unsigned i = 2; i < NPairState::kNumValues; i++)
        if (as.StateActions[i] != NPairAction::kCompress)
            return E_NOTIMPL;
    return S_OK;
}

void GetUpdateArchiveMTime(const CDirItems &dirItems, const CObjectVector<CArcItem> &arcItems,
                           const CRecordVector<CUpdatePair2> &updatePairs2, CFiTime &ft,
                           bool &ft_Defined) {
    FiTime_Clear(ft);
    ft_Defined = false;
    FOR_VECTOR(i, updatePairs2) {
        const CUpdatePair2 &pair2 = updatePairs2[i];
        CFiTime ft2;
        FiTime_Clear(ft2);
        bool ft2_Defined = false;
        /* we use full precision of dirItem, if dirItem is defined
           and (dirItem will be used or dirItem is sameTime in dir and arc */
        if (pair2.DirIndex >= 0 && (pair2.NewProps || pair2.IsSameTime)) {
            ft2 = dirItems.Items[(unsigned)pair2.DirIndex].MTime;
            ft2_Defined = true;
        } else if (pair2.UseArcProps && pair2.ArcIndex >= 0) {
            const CArcItem &arcItem = arcItems[(unsigned)pair2.ArcIndex];
            if (arcItem.MTime.Def) {
                arcItem.MTime.Write_To_FiTime(ft2);
                ft2_Defined = true;
            }
        }
        if (ft2_Defined) {
            if (!ft_Defined || Compare_FiTime(&ft, &ft2) < 0) {
                ft = ft2;
                ft_Defined = true;
            }
        }
    }
}

HRESULT UpdateArchive(const CDirItems &dirItems, const CByteBuffer &processedItems,
                      const CBoolPair &SymLinks, IUpdateDeletionCallback *callback) {
    if (!callback || processedItems.Size() != dirItems.Items.Size())
        return E_INVALIDARG;
    CRecordVector<CDirPathSortPair> pairs;
    FStringVector foldersNames;

    unsigned i;

    for (i = 0; i < dirItems.Items.Size(); i++) {
        const CDirItem &dirItem = dirItems.Items[i];
        const FString phyPath = dirItems.GetPhyPath(i);
        if (dirItem.IsDir()) {
            CDirPathSortPair pair;
            pair.Index = i;
            pair.SetNumSlashes(phyPath);
            pairs.Add(pair);
        } else {
            // 21.04: we have set processedItems[*] before for all required items
            if (processedItems[i] != 0
                // || dirItem.Size == 0
                // || dirItem.AreReparseData()
            ) {
                NFind::CFileInfo fileInfo;
                /* if (!SymLinks), we follow link here, similar to (dirItem) filling */
                if (fileInfo.Find(phyPath, !SymLinks.Val)) {
                    bool is_SameSize = false;
                    if (SymLinks.Val && dirItem.AreReparseData()) {
                        /* (dirItem.Size = dirItem.ReparseData.Size()) was set before.
                           So we don't compare sizes for that case here */
                        is_SameSize = fileInfo.IsOsSymLink();
                    } else
                        is_SameSize = (fileInfo.Size == dirItem.Size);

                    if (is_SameSize && Compare_FiTime(&fileInfo.MTime, &dirItem.MTime) == 0 &&
                        Compare_FiTime(&fileInfo.CTime, &dirItem.CTime) == 0) {
                        const HRESULT result = callback->DeletingAfterArchiving(phyPath, false);
                        if (result != S_OK)
                            return result;
                        DeleteFileAlways(phyPath);
                    }
                }
            } else {
                // file was skipped by some reason. We can throw error for debug:
                /*
                errorInfo.SystemError = 0;
                errorInfo.Message = "file was not processed";
                errorInfo.FileNames.Add(phyPath);
                return E_FAIL;
                */
            }
        }
    }

    pairs.Sort2();

    for (i = 0; i < pairs.Size(); i++) {
        const FString phyPath = dirItems.GetPhyPath(pairs[i].Index);
        if (NFind::DoesDirExist(phyPath)) {
            const HRESULT result = callback->DeletingAfterArchiving(phyPath, true);
            if (result != S_OK)
                return result;
            RemoveDirAlways_if_Empty(phyPath);
        }
    }

    const HRESULT result = callback->FinishDeletingAfterArchiving();
    if (result != S_OK)
        return result;
    return S_OK;
}
