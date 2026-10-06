// PanelOperations.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Filesystem Create/Rename/Delete portions; archive mutations remain P1-04.

#include "7zip/UI/Common/StdAfx.h"

#include "7zip/UI/FileManager/resource.h"
#include "Common/StringConvert.h"

#include "BrowseDialog.h"
#include "ComboDialog.h"
#include "FSFolder.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "Panel.h"
#include "UpdateCallback100.h"
#include "Windows/ErrorMsg.h"
#include "qtfolderpropertycapabilities.h"
#include "qtprogresstransport.h"
#include "qttrashservice.h"

#include <QFileInfo>
#include <QMessageBox>

#ifdef _WIN32
#include "Windows/COM.h"
#endif

static const UInt32 kParentIndex = (UInt32)-1;

enum EFolderOpType {
    FOLDER_TYPE_CREATE_FOLDER = 0,
    FOLDER_TYPE_DELETE = 1,
    FOLDER_TYPE_RENAME = 2,
    FOLDER_TYPE_CREATE_FILE = 3, // Source CreateFile calls IFolderOperations directly.
    FOLDER_TYPE_COMMENT = 4
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
    case FOLDER_TYPE_COMMENT: {
        NWindows::NCOM::CPropVariant value(Name);
        return FolderOperations->SetProperty(Index, kpidComment, &value, UpdateCallback);
    }
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

    if (!panel.GetParentFolders().IsEmpty()) {
        const CFolderLink &folderLink = panel.GetParentFolders().Back();
        UpdateCallbackSpec->PasswordIsDefined = folderLink.UsePassword;
        UpdateCallbackSpec->Password = folderLink.Password;
    }

    MainWindow = panel.GetParent();
    MainTitle = "7-Zip";
    MainAddTitle = progressTitle + L' ';

    std::unique_ptr<IQtProgressTransport> transport;
    if (auto *services = dynamic_cast<IQtProgressServices *>(FolderOperations.Interface()))
        transport = OpType == FOLDER_TYPE_COMMENT ? services->CreateNativeProgressTransport(*this)
                                                  : services->CreateProgressTransport(*this);
    RINOK(Create(progressTitle, MainWindow))
    return Result;
}

static bool IsCorrectFsName(const UString &name) {
    const UString lastPart = name.Ptr((unsigned)(name.ReverseFind_PathSepar() + 1));
    return !lastPart.IsEqualTo(".") && !lastPart.IsEqualTo("..");
}

bool CPanel::CanComment() const {
    const int focused = _selectionList.GetFocusedItem();
    if (focused < 0 || GetRealItemIndex(focused) == kParentIndex || IsThereReadOnlyFolder())
        return false;
    const auto *folder = dynamic_cast<IQtFolderPropertyCapabilities *>(_folder.Interface());
    return folder && folder->CanComment();
}

bool CPanel::ChangeComment(CSelectedState &state) {
    if (IsHashFolder())
        return false;
    if (!CheckBeforeUpdate(IDS_COMMENT))
        return false;
    CDisableTimerProcessing disableTimerProcessing2(*this);
    const int index = _selectionList.GetFocusedItem();
    if (index < 0)
        return false;
    const unsigned realIndex = GetRealItemIndex(index);
    if (realIndex == kParentIndex)
        return false;
    SaveSelectedState(state);
    UString comment;
    {
        NWindows::NCOM::CPropVariant propVariant;
        if (_folder->GetProperty(realIndex, kpidComment, &propVariant) != S_OK)
            return false;
        if (propVariant.vt == VT_BSTR)
            comment = propVariant.bstrVal;
        else if (propVariant.vt != VT_EMPTY)
            return false;
    }
    const UString name = GetItemRelPath2(realIndex);
    UString title = name;
    title += " : ";
    AddLangString(title, IDS_COMMENT);
    CComboDialog dlg(ToQString(title), ToQString(LangString(IDS_COMMENT2)), ToQString(comment),
                     GetParent());
    dlg.BindLanguage(0, IDS_COMMENT2);
    if (dlg.exec() != QDialog::Accepted)
        return false;

    CDisableNotify disableNotify(*this);
    // Qt runs the unchanged source property operation on its existing worker
    // and callback lifecycle. The caller owns the original error presentation.
    CThreadFolderOperations op(FOLDER_TYPE_COMMENT);
    op.FolderOperations = _folderOperations;
    op.Index = realIndex;
    op.Name = ToUString(dlg.Value);
    op.DisableUserQuestions = true;
    const HRESULT result = op.DoOperation(*this, LangString(IDS_COMMENT), L"Set Comment Error");
    if (result != S_OK && result != E_ABORT) {
        if (result == E_NOINTERFACE)
            MessageBox_Error_UnsupportOperation();
        else {
            QMessageBox box(QMessageBox::Critical, QStringLiteral("Set Comment Error"),
                            ToQString(NWindows::NError::MyFormatMessage(result)), QMessageBox::Ok,
                            GetParent());
            box.setObjectName("commentUpdateError");
#ifdef Q_OS_MACOS
            // Native macOS message boxes omit window captions. Keep the
            // original source caption visible as the message heading.
            box.setInformativeText(box.text());
            box.setText(QStringLiteral("Set Comment Error"));
#endif
            box.setTextFormat(Qt::PlainText);
            box.exec();
        }
    }
    // The original reaches RefreshListCtrl here even after operation errors.
    // Qt must load the provider before restoring this captured source state.
    return true;
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

HRESULT CPanel::DeleteItems(bool toRecycleBin, IQtTrashService &trash, CSelectedState &state,
                            const CRecordVector<UInt32> *explicitIndices) {
    CDisableTimerProcessing disableTimerProcessing(*this);
    CRecordVector<UInt32> indices;
    if (explicitIndices)
        indices = *explicitIndices;
    else
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
