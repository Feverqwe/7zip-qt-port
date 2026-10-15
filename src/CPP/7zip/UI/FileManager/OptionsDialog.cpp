// Adapted from 7-Zip OptionsDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt property-sheet container: original page order and initial System page.
#include "7zip/UI/FileManager/resource.h"
#include "EditPage.h"
#include "FoldersPage.h"
#include "LangPage.h"
#include "LangUtils.h"
#include "MenuPage.h"
#include "SettingsPage.h"
#include "SystemPage.h"
#include "qtassociationplatform.h"
#include "qtdialogunits.h"
#include "qtfmoptionsdialog.h"
#include "qtlangcontrols.h"
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QShortcut>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

QtFmOptionsDialog::QtFmOptionsDialog(const QString &settingsPath, QWidget *parent,
                                     IFolderManager *manager,
                                     std::shared_ptr<IQtAssociationServices> associations)
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
    if (!associations)
        associations = CreateQtAssociationServices(CreateNativeAssociationPlatform());
    _systemPage = new CSystemPage(manager, std::move(associations), pages);
    // OptionsDialog's pageIDs/pagePointers, retaining source order and IDs.
    const unsigned pageIDs[] = {2200, 2300, 2400, 2103, 2500, 2101};
    QWidget *pagePointers[] = {_systemPage, _menuPage,     _foldersPage,
                               _editPage,   _settingsPage, _langPage};
    const QString titles[] = {"System", "7-Zip", "Folders", "Editor", "Settings", "Language"};
    for (unsigned i = 0; i < 6; i++) {
        auto *page = pagePointers[i] ? pagePointers[i] : new QWidget(pages);
        page->setProperty("sourcePageID", pageIDs[i]);
        pages->addTab(page, QtLangText(pageIDs[i], titles[i]));
        pages->setTabEnabled(int(i), pagePointers[i] != nullptr);
    }
    // MyPropertySheet sets nStartPage = 0 for both native character variants.
    pages->setCurrentIndex(0);
    layout->addWidget(pages);
    _buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                                        QDialogButtonBox::Apply | QDialogButtonBox::Help,
                                    this);
    _buttons->setObjectName("settingsButtons");
    layout->addWidget(_buttons);
    connect(_buttons, &QDialogButtonBox::accepted, this, &QtFmOptionsDialog::accept);
    connect(_buttons, &QDialogButtonBox::rejected, this, &QtFmOptionsDialog::reject);
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
    connect(_systemPage, &CSystemPage::Changed, this, &QtFmOptionsDialog::updateApplyButton);
    updateApplyButton();
    // MyPropertySheet uses the largest source page: GuiCommon.rc's 300x280
    // content DLU plus two eight-DLU margins. Qt supplies its native chrome.
    QSize pageHint;
    for (auto *page : pagePointers)
        if (page)
            pageHint = pageHint.expandedTo(page->sizeHint());
    const QSize tabHint = pages->tabBar()->sizeHint();
    const QSize contentHint(qMax(pageHint.width(), tabHint.width()), pageHint.height());
    const QSize tabChrome = (pages->sizeHint() - contentHint).expandedTo(QSize(0, 0));
    QSize sourcePage = QtDialogUnits(font()).pixels(QSize(316, 296));
    sourcePage.setWidth(qMax(sourcePage.width(), tabHint.width()));
    const QMargins margins = layout->contentsMargins();
    const int footerHeight = layout->spacing() + _buttons->sizeHint().height();
    const QSize outer(margins.left() + margins.right(),
                      margins.top() + margins.bottom() + footerHeight);
    resize((sourcePage + tabChrome + outer).expandedTo(minimumSizeHint()));
}

CFmSettings QtFmOptionsDialog::settings() const { return _settingsPage->settings(); }
void QtFmOptionsDialog::showPageHelp() {
    auto *page = findChild<QTabWidget *>("optionsPages")->currentWidget();
    if (page == _systemPage)
        _systemPage->OnNotifyHelp();
    else if (page == _editPage)
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
    if (_applying)
        return;
    QScopedValueRollback<bool> applying(_applying, true);
    setEnabled(false);
    const auto enable = qScopeGuard([this] { setEnabled(true); });
    const LONG associationError = _systemPage->OnApply();
    setEnabled(true);
    if (associationError != 0) {
        const QString message =
            associationError == 1237
                ? QtLocalText("An association changed outside the application. Review the "
                              "refreshed state and try again.",
                              "Ассоциация изменена другим приложением. Проверьте обновлённое "
                              "состояние и повторите действие.")
                : QtLocalText("Unable to change the association (error %1).",
                              "Не удалось изменить ассоциацию (ошибка %1).")
                      .arg(associationError);
        QMessageBox::critical(this, QtLocalText("7-Zip", "7-Zip"), message);
    }
    // Source SystemPage reports an association error and returns
    // PSNRET_NOERROR, so the property sheet still applies the other pages.
    _menuPage->OnApply();
    _foldersPage->OnApply();
    _editPage->OnApply();
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
                     _langPage->IsModified() || _systemPage->IsModified());
}
void QtFmOptionsDialog::reloadLangItems() {
    QtReloadLangTexts(*this);
    _menuPage->ReloadLangItems();
    _systemPage->ReloadLangItems();
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
    if (_applying)
        return;
    apply();
    if (!_settingsPage->IsModified())
        QDialog::accept();
}
void QtFmOptionsDialog::reject() {
    if (!_applying)
        QDialog::reject();
}
void QtFmOptionsDialog::closeEvent(QCloseEvent *event) {
    if (_applying)
        event->ignore();
    else
        QDialog::closeEvent(event);
}
