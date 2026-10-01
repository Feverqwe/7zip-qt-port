// Adapted from 7-Zip FoldersPage.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original work-mode/state/apply bodies; Qt replaces property-page controls.
#include "FoldersPage.h"
#include "7zip/UI/FileManager/FoldersPageRes.h"
#include "BrowseDialog.h"
#include "Common/StringConvert.h"
#include "Panel.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

static const unsigned kWorkModeButtons[] = {IDR_FOLDERS_WORK_SYSTEM, IDR_FOLDERS_WORK_CURRENT,
                                            IDR_FOLDERS_WORK_SPECIFIED};
static const unsigned kNumWorkModeButtons = Z7_ARRAY_SIZE(kWorkModeButtons);

CFoldersPage::CFoldersPage(const QString &settingsPath, QWidget *parent)
    : QWidget(parent), _settingsPath(settingsPath) {
    setObjectName("foldersPage");
    auto *layout = new QGridLayout(this);
    layout->addWidget(new QLabel("Рабочая папка", this), 0, 0, 1, 2);
    _workModes = new QButtonGroup(this);
    const QString labels[] = {"Системная &временная папка", "&Текущая папка", "&Указанная папка:"};
    const char *names[] = {"workSystem", "workCurrent", "workSpecified"};
    for (unsigned i = 0; i < kNumWorkModeButtons; ++i) {
        auto *button = new QRadioButton(labels[i], this);
        button->setObjectName(names[i]);
        button->setProperty("sourceControlID", kWorkModeButtons[i]);
        _workModes->addButton(button, int(kWorkModeButtons[i]));
        layout->addWidget(button, int(i) + 1, 0, 1, 2);
    }
    m_WorkPath = new QLineEdit(this);
    m_WorkPath->setObjectName("workPath");
    m_WorkPath->setAccessibleName("Указанная рабочая папка");
    _workBrowse = new QPushButton("…", this);
    _workBrowse->setObjectName("workPathBrowse");
    _workBrowse->setAccessibleName("Выбрать рабочую папку");
    _workBrowse->setFixedWidth(_workBrowse->sizeHint().height() * 2);
    layout->addWidget(m_WorkPath, 4, 0);
    layout->addWidget(_workBrowse, 4, 1);
    _forRemovable = new QCheckBox("Только для сменных носителей", this);
    _forRemovable->setObjectName("workRemovableOnly");
#ifndef _WIN32
    // Existing source WorkDir drive-type branch is Windows-only. Keep its value
    // without offering an inactive command on POSIX; platform owner P2-05.
    _forRemovable->setEnabled(false);
    _forRemovable->setToolTip("Определение сменных носителей доступно в Windows");
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
    QString title = "Выберите папку для временных файлов архива.";
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
