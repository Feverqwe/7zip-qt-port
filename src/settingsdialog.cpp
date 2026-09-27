#include "settingsdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const FileListSettings &settings, QWidget *parent)
    : QDialog(parent) {
    setObjectName("settingsDialog");
    setWindowTitle("Настройки");
    auto *layout = new QVBoxLayout(this);
    auto *heading = new QLabel("Настройки списка файлов", this);
    layout->addWidget(heading);
    realFileIcons_ = new QCheckBox("Показывать системные значки файлов", this);
    realFileIcons_->setObjectName("realFileIconsOption");
    realFileIcons_->setChecked(settings.realFileIcons);
    layout->addWidget(realFileIcons_);
    grid_ = new QCheckBox("Показывать линии сетки", this);
    grid_->setObjectName("gridOption");
    grid_->setChecked(settings.grid);
    layout->addWidget(grid_);
    singleClick_ = new QCheckBox("Открывать одним щелчком", this);
    singleClick_->setObjectName("singleClickOption");
    singleClick_->setChecked(settings.singleClick);
    layout->addWidget(singleClick_);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

FileListSettings SettingsDialog::settings() const {
    return {realFileIcons_->isChecked(), grid_->isChecked(), singleClick_->isChecked()};
}
