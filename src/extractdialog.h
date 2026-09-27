#pragma once

#include "archivebackend.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

// The controls follow the path and overwrite choices in GUI/ExtractDialog.cpp.
class ExtractDialog : public QDialog {
    Q_OBJECT

  public:
    ExtractDialog(const QString &initialPath, const QStringList &history, bool selected,
                  QWidget *parent = nullptr)
        : QDialog(parent) {
        setObjectName("extractDialog");
        setWindowTitle(selected ? "Извлечь выбранное" : "Извлечь");
        resize(470, 330);
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Извлечь в:", this));
        auto *pathRow = new QHBoxLayout;
        destination_ = new QComboBox(this);
        destination_->setObjectName("extractDestination");
        destination_->setEditable(true);
        destination_->setInsertPolicy(QComboBox::NoInsert);
        destination_->addItems(history);
        destination_->setCurrentText(initialPath);
        pathRow->addWidget(destination_, 1);
        auto *browse = new QPushButton("…", this);
        browse->setObjectName("extractBrowseButton");
        pathRow->addWidget(browse);
        layout->addLayout(pathRow);

        auto *options = new QHBoxLayout;
        auto *modeColumn = new QVBoxLayout;
        modeColumn->addWidget(new QLabel("Пути:", this));
        pathMode_ = new QComboBox(this);
        pathMode_->setObjectName("extractPathMode");
        pathMode_->addItem("Полные пути");
        pathMode_->addItem("Без путей");
        modeColumn->addWidget(pathMode_);
        modeColumn->addWidget(new QLabel("Перезапись:", this));
        overwriteMode_ = new QComboBox(this);
        overwriteMode_->setObjectName("extractOverwriteMode");
        overwriteMode_->addItem("Не перезаписывать существующие файлы");
        overwriteMode_->addItem("Пропускать существующие файлы");
        overwriteMode_->addItem("Перезаписывать существующие файлы");
        modeColumn->addWidget(overwriteMode_);
        options->addLayout(modeColumn, 1);

        auto *passwordGroup = new QGroupBox("Пароль", this);
        auto *passwordLayout = new QVBoxLayout(passwordGroup);
        password_ = new QLineEdit(passwordGroup);
        password_->setObjectName("extractPassword");
        password_->setEchoMode(QLineEdit::Password);
        passwordLayout->addWidget(password_);
        auto *showPassword = new QCheckBox("Показать пароль", passwordGroup);
        showPassword->setObjectName("extractShowPassword");
        passwordLayout->addWidget(showPassword);
        options->addWidget(passwordGroup, 1);
        layout->addLayout(options);
        auto *hint = new QLabel("Папка назначения должна существовать. При выборе «Без путей» "
                                "файлы с одинаковыми именами из разных папок не извлекаются.",
                                this);
        hint->setWordWrap(true);
        layout->addWidget(hint);
        layout->addStretch();
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getExistingDirectory(this, "Папка для извлечения",
                                                                   destination_->currentText());
            if (!path.isEmpty())
                destination_->setCurrentText(path);
        });
        connect(showPassword, &QCheckBox::toggled, password_, [this](bool visible) {
            password_->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
        });
    }

    QString destination() const { return destination_->currentText().trimmed(); }
    QString password() const { return password_->text(); }
    ExtractionOptions options() const {
        ExtractionOptions result;
        if (pathMode_->currentIndex() == 1)
            result.pathMode = ExtractionOptions::PathMode::NoPaths;
        if (overwriteMode_->currentIndex() == 1)
            result.overwriteMode = ExtractionOptions::OverwriteMode::SkipExisting;
        else if (overwriteMode_->currentIndex() == 2)
            result.overwriteMode = ExtractionOptions::OverwriteMode::OverwriteExisting;
        return result;
    }

  private:
    QComboBox *destination_ = nullptr;
    QComboBox *pathMode_ = nullptr;
    QComboBox *overwriteMode_ = nullptr;
    QLineEdit *password_ = nullptr;
};
