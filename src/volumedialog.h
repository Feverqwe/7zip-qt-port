#pragma once

#include "volumeoperations.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class VolumeDialog : public QDialog {
    Q_OBJECT

  public:
    VolumeDialog(VolumeOperation::Mode mode, const QString &source, QWidget *parent = nullptr)
        : QDialog(parent), mode_(mode) {
        setObjectName(mode == VolumeOperation::Mode::Split ? "splitDialog" : "combineDialog");
        setWindowTitle(mode == VolumeOperation::Mode::Split ? "Разбить файл" : "Объединить файлы");
        resize(480, 190);
        auto *layout = new QVBoxLayout(this);
        auto *sourceLabel = new QLabel(QFileInfo(source).fileName(), this);
        sourceLabel->setObjectName("volumeSource");
        layout->addWidget(sourceLabel);
        auto *form = new QFormLayout;
        auto *destinationRow = new QHBoxLayout;
        destination_ = new QLineEdit(QFileInfo(source).absolutePath(), this);
        destination_->setObjectName("volumeDestination");
        destinationRow->addWidget(destination_);
        auto *browse = new QPushButton("…", this);
        browse->setObjectName("volumeBrowse");
        destinationRow->addWidget(browse);
        form->addRow(mode == VolumeOperation::Mode::Split ? "Разбить в:" : "Объединить в:",
                     destinationRow);
        if (mode == VolumeOperation::Mode::Split) {
            size_ = new QComboBox(this);
            size_->setObjectName("volumeSize");
            size_->setEditable(true);
            size_->addItems({"10M", "100M", "1000M", "650M", "700M", "4092M", "4480M", "8128M"});
            form->addRow("Размер тома, байт:", size_);
        }
        layout->addLayout(form);
        error_ = new QLabel(this);
        error_->setObjectName("volumeError");
        layout->addWidget(error_);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, this, [this] {
            if (!QFileInfo(destination_->text()).isDir() ||
                QFileInfo(destination_->text()).isSymLink()) {
                error_->setText("Укажите существующую папку назначения.");
                return;
            }
            if (mode_ == VolumeOperation::Mode::Split && volumeSize() == 0) {
                error_->setText("Укажите положительный размер тома (например, 10M).");
                return;
            }
            accept();
        });
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString path =
                QFileDialog::getExistingDirectory(this, "Папка назначения", destination_->text());
            if (!path.isEmpty())
                destination_->setText(path);
        });
    }

    QString destination() const { return destination_->text(); }
    qint64 volumeSize() const {
        return size_ ? VolumeOperation::parseSize(size_->currentText()) : 0;
    }

  private:
    VolumeOperation::Mode mode_;
    QLineEdit *destination_ = nullptr;
    QComboBox *size_ = nullptr;
    QLabel *error_ = nullptr;
};
