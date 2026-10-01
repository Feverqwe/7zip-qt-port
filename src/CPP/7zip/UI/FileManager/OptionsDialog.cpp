// Adapted from 7-Zip OptionsDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Partial property-sheet container: native page order; unavailable pages disabled.
#include "7zip/UI/FileManager/resource.h"
#include "EditPage.h"
#include "FoldersPage.h"
#include "LangPage.h"
#include "LangUtils.h"
#include "MenuPage.h"
#include "SettingsPage.h"
#include "qtfmoptionsdialog.h"
#include "qtlangcontrols.h"
#include <QDialogButtonBox>
#include <QEvent>
#include <QPushButton>
#include <QShortcut>
#include <QTabWidget>
#include <QVBoxLayout>

QtFmOptionsDialog::QtFmOptionsDialog(const QString &settingsPath, QWidget *parent)
    : QDialog(parent) {
    setObjectName("settingsDialog");
    ReloadLang(settingsPath);
    QtBindLangText(this, IDS_OPTIONS, "Options");
    auto *layout = new QVBoxLayout(this);
    auto *pages = new QTabWidget(this);
    pages->setObjectName("optionsPages");
    _editPage = new CEditPage(settingsPath, pages);
    _foldersPage = new CFoldersPage(settingsPath, pages);
    _settingsPage = new CSettingsPage(settingsPath, pages, true);
    _menuPage = new CMenuPage(settingsPath, pages);
    _langPage = new CLangPage(settingsPath, pages);
    // OptionsDialog's pageIDs/pagePointers, retaining source order and IDs.
    // Missing pages and shell consumers remain mandatory P1-14/P2-08/P2-09.
    const unsigned pageIDs[] = {2200, 2300, 2400, 2103, 2500, 2101};
    QWidget *pagePointers[] = {nullptr,   _menuPage,     _foldersPage,
                               _editPage, _settingsPage, _langPage};
    const QString titles[] = {"System", "7-Zip", "Folders", "Editor", "Settings", "Language"};
    for (unsigned i = 0; i < 6; i++) {
        auto *page = pagePointers[i] ? pagePointers[i] : new QWidget(pages);
        page->setProperty("sourcePageID", pageIDs[i]);
        pages->addTab(page, QtLangText(pageIDs[i], titles[i]));
        pages->setTabEnabled(int(i), pagePointers[i] != nullptr);
    }
    pages->setCurrentWidget(_settingsPage);
    layout->addWidget(pages);
    _buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                                        QDialogButtonBox::Apply | QDialogButtonBox::Help,
                                    this);
    _buttons->setObjectName("settingsButtons");
    layout->addWidget(_buttons);
    connect(_buttons, &QDialogButtonBox::accepted, this, &QtFmOptionsDialog::accept);
    connect(_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(_buttons, &QDialogButtonBox::helpRequested, this, &QtFmOptionsDialog::showPageHelp);
    auto *help = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(help, &QShortcut::activated, this, &QtFmOptionsDialog::showPageHelp);
    connect(_buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
            &QtFmOptionsDialog::apply);
    connect(_editPage, &CEditPage::Changed, this, &QtFmOptionsDialog::updateApplyButton);
    connect(_settingsPage, &CSettingsPage::Changed, this, &QtFmOptionsDialog::updateApplyButton);
    connect(_foldersPage, &CFoldersPage::Changed, this, &QtFmOptionsDialog::updateApplyButton);
    connect(_menuPage, &CMenuPage::Changed, this, &QtFmOptionsDialog::updateApplyButton);
    connect(_langPage, &CLangPage::Changed, this, &QtFmOptionsDialog::updateApplyButton);
    updateApplyButton();
    resize(640, 390);
}

CFmSettings QtFmOptionsDialog::settings() const { return _settingsPage->settings(); }
void QtFmOptionsDialog::showPageHelp() {
    auto *page = findChild<QTabWidget *>("optionsPages")->currentWidget();
    if (page == _editPage)
        _editPage->OnNotifyHelp();
    else if (page == _foldersPage)
        _foldersPage->OnNotifyHelp();
    else if (page == _settingsPage)
        _settingsPage->OnNotifyHelp();
    else if (page == _menuPage)
        _menuPage->OnNotifyHelp();
    else if (page == _langPage)
        _langPage->OnNotifyHelp();
}
void QtFmOptionsDialog::apply() {
    _editPage->OnApply();
    _foldersPage->OnApply();
    _menuPage->OnApply();
    if (_settingsPage->OnApply() != 0) {
        findChild<QTabWidget *>("optionsPages")->setCurrentWidget(_settingsPage);
        _settingsPage->setFocus();
        updateApplyButton();
        return;
    }
    const bool languageChanged = _langPage->IsModified();
    _langPage->OnApply();
    if (languageChanged)
        QtNotifyLanguageChanged();
    if (Applied)
        Applied(settings());
    updateApplyButton();
}
void QtFmOptionsDialog::updateApplyButton() {
    _buttons->button(QDialogButtonBox::Apply)
        ->setEnabled(_editPage->IsModified() || _foldersPage->IsModified() ||
                     _settingsPage->IsModified() || _menuPage->IsModified() ||
                     _langPage->IsModified());
}
void QtFmOptionsDialog::reloadLangItems() {
    QtReloadLangTexts(*this);
    _menuPage->ReloadLangItems();
    auto *pages = findChild<QTabWidget *>("optionsPages");
    const QString titles[] = {"System", "7-Zip", "Folders", "Editor", "Settings", "Language"};
    for (int i = 0; i < pages->count(); i++)
        pages->setTabText(
            i, QtLangText(pages->widget(i)->property("sourcePageID").toUInt(), titles[i]));
}
void QtFmOptionsDialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange && _langPage)
        reloadLangItems();
    QDialog::changeEvent(event);
}
void QtFmOptionsDialog::accept() {
    apply();
    if (!_settingsPage->IsModified())
        QDialog::accept();
}
