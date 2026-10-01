// Adapted from 7-Zip EditPage.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt supplies controls/property-page services; original state/apply/Browse flow.
#include "EditPage.h"
#include "7zip/UI/FileManager/EditPageRes.h"
#include "BrowseDialog.h"
#include "Panel.h"
#include "RegistryUtils.h"
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

CEditPage::CEditPage(const QString &settingsPath, QWidget *parent)
    : QWidget(parent), _settingsPath(settingsPath) {
    setObjectName("editorPage");
    auto *layout = new QGridLayout(this);
    const QString labels[] = {"&Просмотр:", "&Редактор:", "&Сравнение:"};
    const char *names[] = {"viewerCommand", "editorCommand", "diffCommand"};
    const unsigned edits[] = {IDE_EDIT_VIEWER, IDE_EDIT_EDITOR, IDE_EDIT_DIFF};
    const unsigned buttons[] = {IDB_EDIT_VIEWER, IDB_EDIT_EDITOR, IDB_EDIT_DIFF};
    for (unsigned i = 0; i < 3; ++i) {
        auto *label = new QLabel(labels[i], this);
        auto &c = _ctrls[i];
        c.Ctrl = edits[i];
        c.Button = buttons[i];
        c.Edit = new QLineEdit(this);
        c.Edit->setObjectName(names[i]);
        c.Edit->setAccessibleName(labels[i].mid(1));
        label->setBuddy(c.Edit);
        auto *browse = new QPushButton("…", this);
        browse->setObjectName(QString(names[i]) + "Browse");
        browse->setAccessibleName(QString("Выбрать приложение: %1").arg(labels[i].mid(1)));
        browse->setFixedWidth(browse->sizeHint().height() * 2);
        layout->addWidget(label, int(i) * 2, 0, 1, 2);
        layout->addWidget(c.Edit, int(i) * 2 + 1, 0);
        layout->addWidget(browse, int(i) * 2 + 1, 1);
        connect(c.Edit, &QLineEdit::textChanged, this, [this, id = c.Ctrl] { OnCommand(id); });
        connect(browse, &QPushButton::clicked, this,
                [this, id = c.Button] { OnButtonClicked(id); });
    }
    layout->setRowStretch(6, 1);
    layout->setColumnStretch(0, 1);
    OnInit();
}

bool CEditPage::OnInit() {
    _initMode = true;
    for (unsigned i = 0; i < 3; i++) {
        CEditPageCtrl &c = _ctrls[i];
        c.WasChanged = false;
        UString path;
        if (i < 2)
            ReadRegEditor(i > 0, path, _settingsPath);
        else
            ReadRegDiff(path, _settingsPath);
        c.Edit->setText(ToQString(path));
    }
    _initMode = false;
    return true;
}

long CEditPage::OnApply() {
    for (unsigned i = 0; i < 3; i++) {
        CEditPageCtrl &c = _ctrls[i];
        if (c.WasChanged) {
            UString path = ToUString(c.Edit->text());
            if (i < 2)
                SaveRegEditor(i > 0, path, _settingsPath);
            else
                SaveRegDiff(path, _settingsPath);
            c.WasChanged = false;
        }
    }
    return 0;
}

static void Edit_BrowseForFile(QLineEdit &edit, QWidget *hwnd) {
    UString cmd = ToUString(edit.text());
    UString param;
    UString prg;
    SplitCmdLineSmart(cmd, prg, param);
    CBrowseInfo bi;
    bi.FilterIndex = 0;
    bi.FilePath = ToQString(prg);
    bi.Owner = hwnd;
#ifdef Q_OS_WIN
    bi.Filters = "Приложения (*.exe);;Все файлы (*)";
#else
    bi.Filters = "Приложения (*)";
#endif
    if (bi.BrowseForFile()) {
        cmd = ToUString(bi.FilePath);
        cmd.Trim();
        edit.setText(ToQString(cmd));
    }
}

bool CEditPage::OnButtonClicked(unsigned buttonID) {
    for (unsigned i = 0; i < 3; i++) {
        CEditPageCtrl &c = _ctrls[i];
        if (buttonID == c.Button) {
            Edit_BrowseForFile(*c.Edit, this);
            return true;
        }
    }
    return false;
}

bool CEditPage::OnCommand(unsigned itemID) {
    // Only QLineEdit::textChanged invokes this EN_CHANGE projection.
    if (!_initMode) {
        for (unsigned i = 0; i < 3; i++) {
            CEditPageCtrl &c = _ctrls[i];
            if (itemID == c.Ctrl) {
                c.WasChanged = true;
                Changed();
                return true;
            }
        }
    }
    return false;
}

bool CEditPage::IsModified() const {
    for (const auto &c : _ctrls)
        if (c.WasChanged)
            return true;
    return false;
}
