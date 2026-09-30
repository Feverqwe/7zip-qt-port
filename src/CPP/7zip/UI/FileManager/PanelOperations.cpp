// PanelOperations.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Filesystem Create/Rename/Delete portions; archive mutations remain P1-04.

#include "7zip/UI/Common/StdAfx.h"

#include "7zip/UI/FileManager/resource.h"
#include "Common/StringConvert.h"

#include "BrowseDialog.h"
#include "FSFolder.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "Panel.h"
#include "UpdateCallback100.h"
#include "qttrashservice.h"

#include <QFileInfo>
#include <QMessageBox>

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

HRESULT CPanel::DeleteItems(bool toRecycleBin, IQtTrashService &trash, CSelectedState &state) {
    CDisableTimerProcessing disableTimerProcessing(*this);
    CRecordVector<UInt32> indices;
    Get_ItemIndices_Operated(indices);
    if (indices.IsEmpty())
        return S_FALSE;
    SaveSelectedState(state);

    if (!CheckBeforeUpdate(IDS_ERROR_DELETING))
        return E_ABORT;

    UInt32 titleID, messageID;
    UString messageParam;
    if (indices.Size() == 1) {
        const unsigned index = indices[0];
        messageParam = GetItemRelPath2(index);
        if (IsItem_Folder(index)) {
            titleID = IDS_CONFIRM_FOLDER_DELETE;
            messageID = IDS_WANT_TO_DELETE_FOLDER;
        } else {
            titleID = IDS_CONFIRM_FILE_DELETE;
            messageID = IDS_WANT_TO_DELETE_FILE;
        }
    } else {
        titleID = IDS_CONFIRM_ITEMS_DELETE;
        messageID = IDS_WANT_TO_DELETE_ITEMS;
        messageParam = NumberToString(indices.Size());
    }
    // The source shell branch presents the platform confirmation itself. Qt's
    // native trash API does not, so use the same source prompt for both modes.
    const QString title = ToQString(LangString(titleID));
    const QString message = ToQString(MyFormatNew(messageID, messageParam));
    if (QMessageBox::question(GetParent(), title, message,
                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                              QMessageBox::No) != QMessageBox::Yes)
        return E_ABORT;

    HRESULT result = S_OK;
    {
        CDisableNotify disableNotify(*this);
        if (toRecycleBin && IsFSFolder()) {
            FOR_VECTOR(i, indices) {
                const QString path = ToQString(GetItemFullPath(indices[i]));
                if (QFileInfo(path).isSymLink()) {
                    result = E_NOTIMPL;
                    break;
                }
                if (!trash.MoveToTrash(path)) {
                    result = E_FAIL;
                    break;
                }
            }
        } else {
            CThreadFolderOperations op(FOLDER_TYPE_DELETE);
            op.FolderOperations = _folderOperations;
            op.Indices = indices;
            auto *folder = dynamic_cast<NFsFolder::CFSFolder *>((IFolderFolder *)_folder);
            if (folder)
                folder->CheckBreak = [&op](const QString &path) {
                    op.Sync.Set_FilePath(path.toStdWString().c_str());
                    return op.Sync.CheckStop() == S_OK;
                };
            result =
                op.DoOperation(*this, LangString(IDS_DELETING), LangString(IDS_ERROR_DELETING));
            if (folder)
                folder->CheckBreak = {};
        }
    }
    RefreshTitleAlways();
    // The Qt presentation reloads FSFolder before restoring this source state.
    return result;
}
