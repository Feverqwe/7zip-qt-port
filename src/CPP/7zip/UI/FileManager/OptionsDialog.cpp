// Adapted from 7-Zip OptionsDialog.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Partial property-sheet container: native page order; unavailable pages disabled.
#include "EditPage.h"
#include "SettingsPage.h"
#include "qtfmoptionsdialog.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

QtFmOptionsDialog::QtFmOptionsDialog(const QString &settingsPath, QWidget *parent)
    : QDialog(parent) {
    setObjectName("settingsDialog");
    setWindowTitle("Настройки");
    auto *layout = new QVBoxLayout(this);
    auto *pages = new QTabWidget(this);
    pages->setObjectName("optionsPages");
    _editPage = new CEditPage(settingsPath, pages);
    _settingsPage = new CSettingsPage(settingsPath, pages, true);
    // OptionsDialog's pageIDs/pagePointers, retaining source order and IDs.
    // Four missing page bodies remain mandatory under P1-12/P1-13/P2-08.
    const unsigned pageIDs[] = {2200, 2300, 2400, 2103, 2500, 2101};
    QWidget *pagePointers[] = {nullptr, nullptr, nullptr, _editPage, _settingsPage, nullptr};
    const QString titles[] = {"Система", "Меню", "Папки", "Редактор", "Настройки", "Язык"};
    for (unsigned i = 0; i < 6; i++) {
        auto *page = pagePointers[i] ? pagePointers[i] : new QWidget(pages);
        page->setProperty("sourcePageID", pageIDs[i]);
        pages->addTab(page, titles[i]);
        pages->setTabEnabled(int(i), pagePointers[i] != nullptr);
    }
    pages->setCurrentWidget(_settingsPage);
    layout->addWidget(pages);
    _buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    _buttons->setObjectName("settingsButtons");
    layout->addWidget(_buttons);
    connect(_buttons, &QDialogButtonBox::accepted, this, &QtFmOptionsDialog::accept);
    connect(_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(_buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
            &QtFmOptionsDialog::apply);
    connect(_editPage, &CEditPage::Changed, this, &QtFmOptionsDialog::updateApplyButton);
    connect(_settingsPage, &CSettingsPage::Changed, this, &QtFmOptionsDialog::updateApplyButton);
    updateApplyButton();
    resize(640, 390);
}

CFmSettings QtFmOptionsDialog::settings() const { return _settingsPage->settings(); }
void QtFmOptionsDialog::apply() {
    _editPage->OnApply();
    _settingsPage->OnApply();
    if (Applied)
        Applied(settings());
    updateApplyButton();
}
void QtFmOptionsDialog::updateApplyButton() {
    _buttons->button(QDialogButtonBox::Apply)
        ->setEnabled(_editPage->IsModified() || _settingsPage->IsModified());
}
void QtFmOptionsDialog::accept() {
    apply();
    QDialog::accept();
}
