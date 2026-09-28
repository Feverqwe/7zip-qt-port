#include "volumedialog.h"
#include "CPP/7zip/UI/FileManager/SplitUtils.h"

#include <QFileInfo>
#include <QHBoxLayout>

VolumeDialog::VolumeDialog(VolumeOperation::Mode mode, const QString &source, QWidget *parent)
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
        AddVolumeItems(*size_);
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
