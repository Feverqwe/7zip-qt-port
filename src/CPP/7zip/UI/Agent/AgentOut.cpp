// Adapted from 7-Zip CPP/7zip/UI/Agent/AgentOut.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source SetFolder/UpdateOneFile/CommonUpdate prerequisites; remaining operations
// are mandatory and recorded in the audit, not accepted by this partial port.
#include "../../../Common/Wildcard.h"
#include "Agent.h"
#include "UpdateCallbackAgent.h"

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
