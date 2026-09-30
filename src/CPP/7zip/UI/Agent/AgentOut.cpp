// Adapted from 7-Zip CPP/7zip/UI/Agent/AgentOut.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source SetFolder/CommonUpdate and file, delete, folder, rename update plans.
// Other mandatory operations remain recorded in the audit.
#include "../../../Common/Wildcard.h"
#include "7zip/Archive/Common/ItemNameUtils.h"
#include "Agent.h"
#include "UpdateCallbackAgent.h"
#include "Windows/TimeUtils.h"

HRESULT CAgent::SetFolder(IFolderFolder *folder) {
    _updatePathPrefix.Empty();
    _updatePathPrefix_is_AltFolder = false;
    _agentFolder = NULL;
    if (!folder)
        return S_OK;
    {
        Z7_DECL_CMyComPtr_QI_FROM(IArchiveFolderInternal, afi, folder) if (afi) {
            RINOK(afi->GetAgentFolder(&_agentFolder))
        }
        if (!_agentFolder)
            return E_FAIL;
    }
    if (_proxy2) {
        _updatePathPrefix = _proxy2->GetDirPath_as_Prefix(_agentFolder->_proxyDirIndex);
        _updatePathPrefix_is_AltFolder = _proxy2->IsAltDir(_agentFolder->_proxyDirIndex);
    } else
        _updatePathPrefix = _proxy->GetDirPath_as_Prefix(_agentFolder->_proxyDirIndex);
    return S_OK;
}

static void SetInArchiveInterfaces(CAgent *agent, CArchiveUpdateCallback *upd) {
    if (agent->_archiveLink.Arcs.IsEmpty())
        return;
    const CArc &arc = agent->GetArc();
    upd->Arc = &arc;
    upd->Archive = arc.Archive;
    upd->ArcFileName = ExtractFileNameFromPath(arc.Path);
}

HRESULT CAgent::CommonUpdate(ISequentialOutStream *outArchiveStream, unsigned numUpdateItems,
                             IArchiveUpdateCallback *updateCallback) {
    if (!CanUpdate())
        return E_NOTIMPL;
    CMyComPtr<IOutArchive> outArchive;
    RINOK(GetArchive()->QueryInterface(IID_IOutArchive, (void **)&outArchive))
    return outArchive->UpdateItems(outArchiveStream, numUpdateItems, updateCallback);
}

HRESULT CAgent::DeleteItems(ISequentialOutStream *outArchiveStream, const UInt32 *indices,
                            UInt32 numItems, IFolderArchiveUpdateCallback *updateCallback100) {
    if (!CanUpdate())
        return E_NOTIMPL;
    CRecordVector<CUpdatePair2> updatePairs;
    CUpdateCallbackAgent updateCallbackAgent;
    updateCallbackAgent.SetCallback(updateCallback100);
    CMyComPtr2_Create<IArchiveUpdateCallback, CArchiveUpdateCallback> updateCallback;

    CUIntVector realIndices;
    _agentFolder->GetRealIndices(indices, numItems,
                                 true,  // includeAltStreams
                                 false, // includeFolderSubItemsInFlatMode
                                 realIndices);
    unsigned curIndex = 0;
    UInt32 numItemsInArchive;
    RINOK(GetArchive()->GetNumberOfItems(&numItemsInArchive))

    UString deletePath;
    for (UInt32 i = 0; i < numItemsInArchive; i++) {
        if (curIndex < realIndices.Size())
            if (realIndices[curIndex] == i) {
                RINOK(GetArc().GetItem_Path2(i, deletePath))
                RINOK(updateCallback100->DeleteOperation(deletePath))
                curIndex++;
                continue;
            }
        CUpdatePair2 up2;
        up2.SetAs_NoChangeArcItem(i);
        updatePairs.Add(up2);
    }
    updateCallback->UpdatePairs = &updatePairs;
    SetInArchiveInterfaces(this, updateCallback.ClsPtr());
    updateCallback->Callback = &updateCallbackAgent;
    return CommonUpdate(outArchiveStream, updatePairs.Size(), updateCallback);
}

HRESULT CAgent::CreateFolder(ISequentialOutStream *outArchiveStream, const wchar_t *folderName,
                             IFolderArchiveUpdateCallback *updateCallback100) {
    if (!CanUpdate())
        return E_NOTIMPL;
    CRecordVector<CUpdatePair2> updatePairs;
    CDirItems dirItems;
    CUpdateCallbackAgent updateCallbackAgent;
    updateCallbackAgent.SetCallback(updateCallback100);
    CMyComPtr2_Create<IArchiveUpdateCallback, CArchiveUpdateCallback> updateCallback;

    UInt32 numItemsInArchive;
    RINOK(GetArchive()->GetNumberOfItems(&numItemsInArchive))
    for (UInt32 i = 0; i < numItemsInArchive; i++) {
        CUpdatePair2 up2;
        up2.SetAs_NoChangeArcItem(i);
        updatePairs.Add(up2);
    }
    CUpdatePair2 up2;
    up2.Construct();
    up2.NewData = up2.NewProps = true;
    up2.UseArcProps = false;
    up2.DirIndex = 0;
    updatePairs.Add(up2);
    updatePairs.ReserveDown();

    CDirItem di;
    di.SetAsDir();
    di.Size = 0;
    if (_proxy2)
        di.Name = _proxy2->GetDirPath_as_Prefix(_agentFolder->_proxyDirIndex);
    else
        di.Name = _proxy->GetDirPath_as_Prefix(_agentFolder->_proxyDirIndex);
    di.Name += folderName;
    CFiTime ft;
    NWindows::NTime::GetCurUtc_FiTime(ft);
    di.CTime = di.ATime = di.MTime = ft;
    dirItems.Items.Add(di);

    updateCallback->Callback = &updateCallbackAgent;
    updateCallback->DirItems = &dirItems;
    updateCallback->UpdatePairs = &updatePairs;
    SetInArchiveInterfaces(this, updateCallback.ClsPtr());
    return CommonUpdate(outArchiveStream, updatePairs.Size(), updateCallback);
}

HRESULT CAgent::RenameItem(ISequentialOutStream *outArchiveStream, const UInt32 *indices,
                           UInt32 numItems, const wchar_t *newItemName,
                           IFolderArchiveUpdateCallback *updateCallback100) {
    if (!CanUpdate())
        return E_NOTIMPL;
    if (numItems != 1)
        return E_INVALIDARG;
    if (!_archiveLink.IsOpen)
        return E_FAIL;
    CRecordVector<CUpdatePair2> updatePairs;
    CUpdateCallbackAgent updateCallbackAgent;
    updateCallbackAgent.SetCallback(updateCallback100);
    CMyComPtr2_Create<IArchiveUpdateCallback, CArchiveUpdateCallback> updateCallback;

    CUIntVector realIndices;
    _agentFolder->GetRealIndices(indices, numItems,
                                 true, // includeAltStreams
                                 true, // includeFolderSubItemsInFlatMode
                                 realIndices);
    const UInt32 ind0 = indices[0];
    const int mainRealIndex = _agentFolder->GetRealIndex(ind0);
    const UString fullPrefix = _agentFolder->GetFullPrefix(ind0);
    UString name = _agentFolder->GetName(ind0);
    NArchive::NItemName::NormalizeSlashes_in_FileName_for_OsPath(name);
    const UString oldItemPath = fullPrefix + name;
    const UString newItemPath = fullPrefix + newItemName;
    UStringVector newNames;

    unsigned curIndex = 0;
    UInt32 numItemsInArchive;
    RINOK(GetArchive()->GetNumberOfItems(&numItemsInArchive))
    for (UInt32 i = 0; i < numItemsInArchive; i++) {
        CUpdatePair2 up2;
        up2.SetAs_NoChangeArcItem(i);
        if (curIndex < realIndices.Size())
            if (realIndices[curIndex] == i) {
                up2.NewProps = true;
                RINOK(GetArc().IsItem_Anti(i, up2.IsAnti))
                UString oldFullPath;
                RINOK(GetArc().GetItem_Path2(i, oldFullPath))
                if (!IsPath1PrefixedByPath2(oldFullPath, oldItemPath))
                    return E_INVALIDARG;
                up2.NewNameIndex =
                    (int)newNames.Add(newItemPath + oldFullPath.Ptr(oldItemPath.Len()));
                up2.IsMainRenameItem = (mainRealIndex == (int)i);
                curIndex++;
            }
        updatePairs.Add(up2);
    }
    updateCallback->Callback = &updateCallbackAgent;
    updateCallback->UpdatePairs = &updatePairs;
    updateCallback->NewNames = &newNames;
    SetInArchiveInterfaces(this, updateCallback.ClsPtr());
    return CommonUpdate(outArchiveStream, updatePairs.Size(), updateCallback);
}

HRESULT CAgent::UpdateOneFile(ISequentialOutStream *outArchiveStream, const UInt32 *indices,
                              UInt32 numItems, const wchar_t *diskFilePath,
                              IFolderArchiveUpdateCallback *updateCallback100) {
    if (!CanUpdate())
        return E_NOTIMPL;
    CRecordVector<CUpdatePair2> updatePairs;
    CDirItems dirItems;
    CUpdateCallbackAgent updateCallbackAgent;
    updateCallbackAgent.SetCallback(updateCallback100);
    CMyComPtr2_Create<IArchiveUpdateCallback, CArchiveUpdateCallback> updateCallback;
    UInt32 realIndex;
    {
        CUIntVector realIndices;
        _agentFolder->GetRealIndices(
            indices, numItems,
            false, // includeAltStreams // we update only main stream of file
            false, // includeFolderSubItemsInFlatMode
            realIndices);
        if (realIndices.Size() != 1)
            return E_FAIL;
        realIndex = realIndices[0];
    }
    {
        FStringVector filePaths;
        filePaths.Add(us2fs(diskFilePath));
        dirItems.EnumerateItems2(FString(), UString(), filePaths, NULL);
        if (dirItems.Items.Size() != 1)
            return E_FAIL;
    }
    UInt32 numItemsInArchive;
    RINOK(GetArchive()->GetNumberOfItems(&numItemsInArchive))
    for (UInt32 i = 0; i < numItemsInArchive; i++) {
        CUpdatePair2 up2;
        up2.SetAs_NoChangeArcItem(i);
        if (realIndex == i) {
            up2.DirIndex = 0;
            up2.NewData = true;
            up2.NewProps = true;
            up2.UseArcProps = false;
        }
        updatePairs.Add(up2);
    }
    updateCallback->DirItems = &dirItems;
    updateCallback->Callback = &updateCallbackAgent;
    updateCallback->UpdatePairs = &updatePairs;
    SetInArchiveInterfaces(this, updateCallback.ClsPtr());
    updateCallback->KeepOriginalItemNames = true;
    return CommonUpdate(outArchiveStream, updatePairs.Size(), updateCallback);
}
