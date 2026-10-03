// Adapted from 7-Zip FoldersPage.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original work-mode/state/apply bodies; Qt replaces property-page controls.
#include "FoldersPage.h"
#include "HelpUtils.h"

#include "7zip/UI/FileManager/FoldersPageRes.h"
#include "BrowseDialog.h"
#include "Common/StringConvert.h"
#include "Panel.h"
#include "qtlangcontrols.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#define kFoldersTopic "fm/options.htm#folders"
void CFoldersPage::OnNotifyHelp() { ShowHelpWindow(kFoldersTopic, this); }

static const unsigned kWorkModeButtons[] = {IDR_FOLDERS_WORK_SYSTEM, IDR_FOLDERS_WORK_CURRENT,
                                            IDR_FOLDERS_WORK_SPECIFIED};
static const unsigned kNumWorkModeButtons = Z7_ARRAY_SIZE(kWorkModeButtons);

CFoldersPage::CFoldersPage(const QString &settingsPath, QWidget *parent)
    : QWidget(parent), _settingsPath(settingsPath) {
    setObjectName("foldersPage");
    auto *layout = new QGridLayout(this);
    auto *workingLabel = new QLabel(this);
    QtBindResourceText(workingLabel, IDT_FOLDERS_WORKING_FOLDER, "&Working folder");
    layout->addWidget(workingLabel, 0, 0, 1, 2);
    _workModes = new QButtonGroup(this);
    const QString labels[] = {"&System temp folder", "&Current", "Specified:"};
    const char *names[] = {"workSystem", "workCurrent", "workSpecified"};
    for (unsigned i = 0; i < kNumWorkModeButtons; ++i) {
        auto *button = new QRadioButton(labels[i], this);
        button->setObjectName(names[i]);
        button->setProperty("sourceControlID", kWorkModeButtons[i]);
        QtBindResourceText(button, kWorkModeButtons[i], labels[i]);
        _workModes->addButton(button, int(kWorkModeButtons[i]));
        layout->addWidget(button, int(i) + 1, 0, 1, 2);
    }
    m_WorkPath = new QLineEdit(this);
    workingLabel->setBuddy(_workModes->button(IDR_FOLDERS_WORK_SYSTEM));
    m_WorkPath->setObjectName("workPath");
    QtBindLangText(m_WorkPath, IDR_FOLDERS_WORK_SPECIFIED, "Specified working folder", true);
    _workBrowse = new QPushButton("…", this);
    _workBrowse->setObjectName("workPathBrowse");
    QtBindLangText(_workBrowse, IDT_FOLDERS_WORKING_FOLDER, "Working folder", true);
    _workBrowse->setFixedWidth(_workBrowse->sizeHint().height() * 2);
    layout->addWidget(m_WorkPath, 4, 0);
    layout->addWidget(_workBrowse, 4, 1);
    _forRemovable = new QCheckBox(this);
    _forRemovable->setProperty("languageWrapWidth", 380);
    QtBindResourceText(_forRemovable, IDX_FOLDERS_WORK_FOR_REMOVABLE,
                       "Use for removable drives only");
    _forRemovable->setObjectName("workRemovableOnly");
#ifndef _WIN32
    // Existing source WorkDir drive-type branch is Windows-only. Keep its value
    // without offering an inactive command on POSIX; platform owner P2-05.
    _forRemovable->setEnabled(false);
    QtBindLocalToolTip(_forRemovable, "Removable-drive detection is available on Windows.",
                       "Определение сменных носителей доступно в Windows.");
#endif
    layout->addWidget(_forRemovable, 5, 0, 1, 2);
    layout->setColumnStretch(0, 1);
    layout->setRowStretch(6, 1);
    connect(_workModes, &QButtonGroup::idClicked, this,
            [this](int id) { OnButtonClicked(unsigned(id)); });
    connect(_workBrowse, &QPushButton::clicked, this,
            [this] { OnButtonClicked(IDB_FOLDERS_WORK_PATH); });
    connect(_forRemovable, &QCheckBox::toggled, this,
            [this] { OnButtonClicked(IDX_FOLDERS_WORK_FOR_REMOVABLE); });
    connect(m_WorkPath, &QLineEdit::textChanged, this, &CFoldersPage::ModifiedEvent);
    OnInit();
}

bool CFoldersPage::OnInit() {
    _initMode = true;
    _needSave = false;
    m_WorkDirInfo.Load(_settingsPath);
    _forRemovable->setChecked(m_WorkDirInfo.ForRemovableOnly);
    _workModes->button(int(kWorkModeButtons[m_WorkDirInfo.Mode]))->setChecked(true);
    m_WorkPath->setText(ToQString(fs2us(m_WorkDirInfo.Path)));
    MyEnableControls();
    _initMode = false;
    return true;
}

int CFoldersPage::GetWorkMode() const {
    for (unsigned i = 0; i < kNumWorkModeButtons; i++)
        if (_workModes->button(int(kWorkModeButtons[i]))->isChecked())
            return (int)i;
    throw 0;
}

void CFoldersPage::MyEnableControls() {
    bool enablePath = (GetWorkMode() == NWorkDir::NMode::kSpecified);
    m_WorkPath->setEnabled(enablePath);
    _workBrowse->setEnabled(enablePath);
}

void CFoldersPage::GetWorkDir(NWorkDir::CInfo &workDirInfo) {
    UString s = ToUString(m_WorkPath->text());
    workDirInfo.Path = us2fs(s);
    workDirInfo.ForRemovableOnly = _forRemovable->isChecked();
    workDirInfo.Mode = NWorkDir::NMode::EEnum(GetWorkMode());
}

void CFoldersPage::ModifiedEvent() {
    if (!_initMode) {
        _needSave = true;
        Changed();
    }
}

bool CFoldersPage::OnButtonClicked(unsigned buttonID) {
    for (unsigned i = 0; i < kNumWorkModeButtons; i++)
        if (buttonID == kWorkModeButtons[i]) {
            MyEnableControls();
            ModifiedEvent();
            return true;
        }
    switch (buttonID) {
    case IDB_FOLDERS_WORK_PATH:
        OnFoldersWorkButtonPath();
        return true;
    case IDX_FOLDERS_WORK_FOR_REMOVABLE:
        break;
    default:
        return false;
    }
    ModifiedEvent();
    return true;
}

void CFoldersPage::OnFoldersWorkButtonPath() {
    QString currentPath = m_WorkPath->text();
    QString title = QtLangText(IDS_FOLDERS_SET_WORK_PATH_TITLE);
    QString resultPath;
    if (MyBrowseForFolder(this, title, currentPath, resultPath))
        m_WorkPath->setText(resultPath);
}

long CFoldersPage::OnApply() {
    if (_needSave) {
        GetWorkDir(m_WorkDirInfo);
        m_WorkDirInfo.Save(_settingsPath);
        _needSave = false;
    }
    return 0;
}
