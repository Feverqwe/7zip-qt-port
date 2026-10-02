#include "7zip/UI/Explorer/resource.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "archiveprogressoperation.h"
#include "mainwindow.h"
#include "qtfilemanagerdrag.h"
#include "qtlangcontrols.h"

bool QtFileManagerDrag::ConfirmCopyToArchive(CPanel &panel, DWORD effect) {
    UString s = LangString(effect == DROPEFFECT_MOVE ? IDS_MOVE_TO : IDS_COPY_TO);
    s.Add_LF();
    s += panel.GetFsPath();
    s.Add_LF();
    AddLangString(s, IDS_WANT_TO_COPY_FILES);
    s += " ?";
    return QMessageBox::question(
               &window_, ToQString(LangString(IDS_CONFIRM_FILE_COPY)), ToQString(s),
               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel) == QMessageBox::Yes;
}
UInt32 QtFileManagerDrag::Drag_OnContextMenu(int x, int y, UInt32 mask) {
    struct Pair {
        UInt32 command;
        unsigned language;
    };
    const Pair pairs[] = {
        {NDragMenu::k_Copy_Base | NDragMenu::k_MenuFlag_Copy, IDS_COPY},
        {NDragMenu::k_Copy_Base | NDragMenu::k_MenuFlag_Move, IDS_MOVE},
        {NDragMenu::k_Copy_ToArc | NDragMenu::k_MenuFlag_Copy, IDS_COPY_TO},
        {NDragMenu::k_AddToArc | NDragMenu::k_MenuFlag_Copy, IDS_CONTEXT_COMPRESS},
        {NDragMenu::k_Cancel, 402}};
    QMenu menu(&window_);
    menu.setObjectName("dragContextMenu");
    window_.setFocus();
    for (const auto &pair : pairs) {
        const UInt32 cmd = pair.command & NDragMenu::k_MenuFlags_CmdMask;
        if (cmd != NDragMenu::k_Cancel && !(mask & (UInt32(1) << cmd)))
            continue;
        UString name = LangString(pair.language);
        if (name.IsEmpty()) {
            if (cmd == NDragMenu::k_Cancel)
                name = "Cancel";
            else
                name.Add_UInt32(pair.language);
        }
        if (cmd == NDragMenu::k_Copy_ToArc) {
            name.Add_Space();
            AddLangString(name, IDS_CONTEXT_ARCHIVE);
        }
        if (cmd == NDragMenu::k_Cancel)
            menu.addSeparator();
        menu.addAction(ToQString(name))->setData(pair.command);
    }
    QAction *chosen = menu.exec({x, y});
    return chosen ? chosen->data().toUInt() : UInt32(NDragMenu::k_Cancel);
}
HRESULT QtFileManagerDrag::CompressFiles(CPanel &, const UString &folder, const UString &name,
                                         const UStringVector &paths, bool) {
    QStringList sources;
    FOR_VECTOR(i, paths) sources.append(ToQString(paths[i]));
    CreateArchiveDialog dialog(ToQString(folder), sources, &window_);
    if (auto *archiveName = dialog.findChild<QLineEdit *>("archiveName")) {
        CCompressDialog path;
        path.Format = dialog.findChild<QComboBox *>("archiveFormat")->currentText();
        path.SetArchiveName(ToQString(name));
        archiveName->setText(path.m_ArchivePath);
    }
    if (dialog.exec() != QDialog::Accepted)
        return E_ABORT;
    ArchiveProgressOperation operation(*window_.backend_, [&] {
        window_.backend_->create(dialog.target(), dialog.sources(), dialog.password(),
                                 dialog.encryptHeaders(), dialog.compressionOptions());
    });
    operation.MainWindow = &window_;
    const HRESULT result = operation.Create(L"7-Zip", &window_);
    return result == S_OK ? operation.Result : result;
}
