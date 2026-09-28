#include "volumedialog.h"
#include "CPP/7zip/UI/FileManager/SplitUtils.h"

#include <QFileInfo>
#include <QHBoxLayout>

VolumeDialog::VolumeDialog(VolumeOperation::Mode mode, const QString &source, QWidget *parent,
                           const VolumeParts &parts)
    : QDialog(parent), mode_(mode) {
    setObjectName(mode == VolumeOperation::Mode::Split ? "splitDialog" : "combineDialog");
    setWindowTitle(mode == VolumeOperation::Mode::Split ? "Разбить файл" : "Объединить файлы");
    resize(480, 190);
    auto *layout = new QVBoxLayout(this);
    auto *sourceLabel = new QLabel(QFileInfo(source).fileName(), this);
    sourceLabel->setTextFormat(Qt::PlainText);
    sourceLabel->setWordWrap(true);
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
    if (mode == VolumeOperation::Mode::Combine && !parts.paths.isEmpty()) {
        QStringList names;
        for (qsizetype i = 0; i < qMin(qsizetype(2), parts.paths.size()); ++i)
            names << QFileInfo(parts.paths[i]).fileName();
        if (parts.paths.size() > 3)
            names << QStringLiteral("…");
        if (parts.paths.size() > 2)
            names << QFileInfo(parts.paths.last()).fileName();
        auto *info = new QLabel(QStringLiteral("Файлов: %1\nРазмер: %2 байт\n\n%3\n%4")
                                    .arg(parts.paths.size())
                                    .arg(parts.totalSize)
                                    .arg(QFileInfo(source).absolutePath(), names.join('\n')),
                                this);
        info->setObjectName("volumePartsInfo");
        info->setWordWrap(true);
        info->setTextFormat(Qt::PlainText);
        info->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(info, 1);
    }
    error_ = new QLabel(this);
    error_->setObjectName("volumeError");
    error_->setWordWrap(true);
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
        if (mode_ == VolumeOperation::Mode::Split && volumeSizes().isEmpty()) {
            error_->setText("Укажите положительные размеры томов (например, 10M или 10M 20M).");
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
