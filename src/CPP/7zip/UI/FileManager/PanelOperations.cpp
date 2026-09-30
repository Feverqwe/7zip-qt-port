// PanelOperations.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Filesystem Create/Rename portions; Delete and archive mutations remain
// P1-03.2 and P1-04 respectively.

#include "7zip/UI/Common/StdAfx.h"

#include "7zip/UI/FileManager/resource.h"
#include "Common/StringConvert.h"

#include "BrowseDialog.h"
#include "LangUtils.h"
#include "Panel.h"
#include "UpdateCallback100.h"

#ifdef _WIN32
#include "CPP/Windows/COM.h"
#endif

static const UInt32 kParentIndex = (UInt32)-1;

enum EFolderOpType {
    FOLDER_TYPE_CREATE_FOLDER = 0,
    FOLDER_TYPE_DELETE = 1,
    FOLDER_TYPE_RENAME = 2,
    FOLDER_TYPE_CREATE_FILE = 3 // Source CreateFile calls IFolderOperations directly.
};

class CThreadFolderOperations : public CProgressThreadVirt {
    HRESULT ProcessVirt() Z7_override;

  public:
    EFolderOpType OpType;
    UString Name;
    UInt32 Index;
    CRecordVector<UInt32> Indices;
    CMyComPtr<IFolderOperations> FolderOperations;
    CMyComPtr<IProgress> UpdateCallback;
    CUpdateCallback100Imp *UpdateCallbackSpec;

    explicit CThreadFolderOperations(EFolderOpType opType) : OpType(opType) {}
    HRESULT DoOperation(CPanel &panel, const UString &progressTitle, const UString &titleError);
};

HRESULT CThreadFolderOperations::ProcessVirt() {
#ifdef _WIN32
    NWindows::NCOM::CComInitializer comInitializer;
#endif
    switch ((int)OpType) {
    case FOLDER_TYPE_CREATE_FOLDER:
        return FolderOperations->CreateFolder(Name, UpdateCallback);
    case FOLDER_TYPE_DELETE:
        return FolderOperations->Delete(Indices.ConstData(), Indices.Size(), UpdateCallback);
    case FOLDER_TYPE_RENAME:
        return FolderOperations->Rename(Index, Name, UpdateCallback);
    case FOLDER_TYPE_CREATE_FILE:
        return FolderOperations->CreateFile(Name, UpdateCallback);
    default:
        return E_FAIL;
    }
}

HRESULT CThreadFolderOperations::DoOperation(CPanel &panel, const UString &progressTitle,
                                             const UString &titleError) {
    UpdateCallbackSpec = new CUpdateCallback100Imp;
    UpdateCallback = UpdateCallbackSpec;
    UpdateCallbackSpec->ProgressDialog = this;

    WaitMode = true;
    Sync.FinalMessage.ErrorMessage.Title = titleError;
    UpdateCallbackSpec->Init();

    MainWindow = panel.GetParent();
    MainTitle = "7-Zip";
    MainAddTitle = progressTitle + L' ';

    RINOK(Create(progressTitle, MainWindow))
    return Result;
}

static bool IsCorrectFsName(const UString &name) {
    const UString lastPart = name.Ptr((unsigned)(name.ReverseFind_PathSepar() + 1));
    return !lastPart.IsEqualTo(".") && !lastPart.IsEqualTo("..");
}

bool CPanel::CorrectFsPath(const UString &path2, UString &result) {
    return ::CorrectFsPath(GetFsPath(), path2, result);
}

HRESULT CPanel::CreateFolder(const UString &enteredName, UString &createdName) {
    if (!CheckBeforeUpdate(IDS_CREATE_FOLDER_ERROR))
        return E_ABORT;
    if (!IsCorrectFsName(enteredName))
        return E_INVALIDARG;
    UString newName = enteredName;
    if (IsFSFolder()) {
        UString correctName;
        if (!CorrectFsPath(newName, correctName))
            return E_INVALIDARG;
        newName = correctName;
    }
    CThreadFolderOperations op(FOLDER_TYPE_CREATE_FOLDER);
    op.FolderOperations = _folderOperations;
    op.Name = newName;
    const HRESULT res =
        op.DoOperation(*this, LangString(IDS_CREATE_FOLDER), LangString(IDS_CREATE_FOLDER_ERROR));
    if (res == S_OK)
        createdName = newName;
    return res;
}

HRESULT CPanel::CreateFile(const UString &enteredName, UString &createdName) {
    if (!CheckBeforeUpdate(IDS_CREATE_FILE_ERROR))
        return E_ABORT;
    UString newName = enteredName;
    if (IsFSFolder()) {
        UString correctName;
        if (!CorrectFsPath(newName, correctName))
            return E_INVALIDARG;
        newName = correctName;
    }
    CThreadFolderOperations op(FOLDER_TYPE_CREATE_FILE);
    op.FolderOperations = _folderOperations;
    op.Name = newName;
    const HRESULT res =
        op.DoOperation(*this, LangString(IDS_CREATE_FILE), LangString(IDS_CREATE_FILE_ERROR));
    if (res == S_OK)
        createdName = newName;
    return res;
}

HRESULT CPanel::OnEndLabelEdit(UInt32 realIndex, const UString &enteredName) {
    if (!CheckBeforeUpdate(IDS_ERROR_RENAMING))
        return E_ABORT;
    if (!IsCorrectFsName(enteredName))
        return E_INVALIDARG;
    UString newName = enteredName;
    if (IsFSFolder()) {
        UString correctName;
        if (!CorrectFsPath(newName, correctName))
            return E_INVALIDARG;
        newName = correctName;
    }
    if (realIndex == kParentIndex)
        return E_INVALIDARG;
    CThreadFolderOperations op(FOLDER_TYPE_RENAME);
    op.FolderOperations = _folderOperations;
    op.Index = realIndex;
    op.Name = newName;
    return op.DoOperation(*this, LangString(IDS_RENAMING), LangString(IDS_ERROR_RENAMING));
}
