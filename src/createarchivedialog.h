#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

class CreateArchiveDialog : public QDialog {
    Q_OBJECT

  public:
    explicit CreateArchiveDialog(const QString &directory, const QStringList &initialSources = {},
                                 QWidget *parent = nullptr, const QString &existingTarget = {})
        : QDialog(parent), currentDirectory_(directory), existingTarget_(existingTarget) {
        setObjectName("createArchiveDialog");
        setWindowTitle(existingTarget_.isEmpty() ? "Создать архив" : "Добавить в архив");
        resize(680, 580);
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(existingTarget_.isEmpty()
                                         ? "Выберите файлы и папки для нового архива"
                                         : "Выберите файлы и папки для открытого архива"));
        auto *navigation = new QHBoxLayout;
        auto *up = new QPushButton("Вверх", this);
        up->setObjectName("sourceUpButton");
        navigation->addWidget(up);
        sourcePath_ = new QLineEdit(currentDirectory_, this);
        sourcePath_->setObjectName("sourceDirectory");
        navigation->addWidget(sourcePath_);
        layout->addLayout(navigation);
        available_ = new QListWidget(this);
        available_->setObjectName("availableSources");
        available_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        layout->addWidget(available_);
        auto *add = new QPushButton("Добавить выбранное", this);
        add->setObjectName("selectSourcesButton");
        layout->addWidget(add);
        layout->addWidget(new QLabel("Источники архива"));
        sources_ = new QListWidget(this);
        sources_->setObjectName("selectedSources");
        layout->addWidget(sources_);
        auto *remove = new QPushButton("Убрать выбранное", this);
        layout->addWidget(remove);
        auto *form = new QFormLayout;
        name_ = new QLineEdit("archive", this);
        name_->setObjectName("archiveName");
        form->addRow("Имя:", name_);
        format_ = new QComboBox(this);
        format_->setObjectName("archiveFormat");
        format_->addItems({"7z", "zip"});
        form->addRow("Формат:", format_);
        if (existingTarget_.isEmpty()) {
            encryption_ = new QComboBox(this);
            encryption_->setObjectName("archiveEncryption");
            form->addRow("Шифрование:", encryption_);
            password_ = new QLineEdit(this);
            password_->setObjectName("archivePassword");
            password_->setEchoMode(QLineEdit::Password);
            form->addRow("Пароль:", password_);
            const auto updateEncryption = [this] {
                encryption_->clear();
                encryption_->addItem("Без шифрования");
                encryption_->addItem("AES-256");
                if (format_->currentText() == "7z")
                    encryption_->addItem("AES-256 и шифрование имён файлов");
            };
            connect(format_, &QComboBox::currentTextChanged, this, updateEncryption);
            connect(encryption_, &QComboBox::currentIndexChanged, this,
                    [this] { password_->setEnabled(encryption_->currentIndex() != 0); });
            updateEncryption();
            password_->setEnabled(false);
        }
        auto *destinationRow = new QHBoxLayout;
        destination_ = new QLineEdit(currentDirectory_, this);
        destination_->setObjectName("archiveDestination");
        destinationRow->addWidget(destination_);
        auto *browse = new QPushButton("Обзор…", this);
        destinationRow->addWidget(browse);
        form->addRow("Место сохранения:", destinationRow);
        layout->addLayout(form);
        if (!existingTarget_.isEmpty()) {
            const QFileInfo target(existingTarget_);
            name_->setText(target.completeBaseName());
            format_->setCurrentText(target.suffix().toLower());
            destination_->setText(target.absolutePath());
            name_->setEnabled(false);
            format_->setEnabled(false);
            destination_->setEnabled(false);
            browse->setEnabled(false);
        }
        preview_ = new QLabel(this);
        preview_->setObjectName("archiveTargetPreview");
        preview_->setWordWrap(true);
        layout->addWidget(preview_);
        buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons_->button(QDialogButtonBox::Ok)
            ->setText(existingTarget_.isEmpty() ? "Создать" : "Добавить");
        buttons_->button(QDialogButtonBox::Cancel)->setText("Отмена");
        layout->addWidget(buttons_);

        connect(up, &QPushButton::clicked, this, [this] {
            QDir parent(currentDirectory_);
            if (parent.cdUp())
                showDirectory(parent.absolutePath());
        });
        connect(sourcePath_, &QLineEdit::returnPressed, this,
                [this] { showDirectory(sourcePath_->text()); });
        connect(available_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
            if (QFileInfo(item->data(Qt::UserRole).toString()).isDir())
                showDirectory(item->data(Qt::UserRole).toString());
        });
        connect(add, &QPushButton::clicked, this, [this] {
            for (QListWidgetItem *item : available_->selectedItems()) {
                const QString path = item->data(Qt::UserRole).toString();
                addSource(path);
            }
        });
        connect(remove, &QPushButton::clicked, this, [this] {
            for (QListWidgetItem *item : sources_->selectedItems()) {
                selected_.removeAll(item->data(Qt::UserRole).toString());
                delete item;
            }
        });
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString path =
                QFileDialog::getExistingDirectory(this, "Место сохранения", destination_->text());
            if (!path.isEmpty())
                destination_->setText(path);
        });
        const auto updatePreview = [this] {
            preview_->setText(existingTarget_.isEmpty()
                                  ? "Новый архив: " + QDir(destination_->text())
                                                          .filePath(name_->text().trimmed() + "." +
                                                                    format_->currentText())
                                  : "Добавить в открытый архив: " + existingTarget_);
        };
        connect(name_, &QLineEdit::textChanged, this, updatePreview);
        connect(destination_, &QLineEdit::textChanged, this, updatePreview);
        connect(format_, &QComboBox::currentTextChanged, this, updatePreview);
        connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttons_, &QDialogButtonBox::accepted, this, [this] {
            if (encryption_ && encryption_->currentIndex() != 0 && password_->text().isEmpty()) {
                QMessageBox::warning(this, "Создать архив", "Введите пароль для шифрования.");
                return;
            }
            const QString base = name_->text().trimmed();
            const QFileInfo destination(destination_->text());
            if (selected_.isEmpty() ||
                (existingTarget_.isEmpty() &&
                 (base.isEmpty() || base == "." || base == ".." || base.contains('/') ||
                  base.contains('\\') || !destination.isDir()))) {
                QMessageBox::warning(this, "Создать архив",
                                     "Выберите источники, имя и существующую папку сохранения.");
                return;
            }
            for (const QString &source : selected_) {
                if (!QFileInfo::exists(source)) {
                    QMessageBox::warning(this, "Создать архив", "Источник не найден: " + source);
                    return;
                }
            }
            target_ = existingTarget_.isEmpty() ? QDir(destination.absoluteFilePath())
                                                      .filePath(base + "." + format_->currentText())
                                                : existingTarget_;
            if (existingTarget_.isEmpty() &&
                (QFileInfo(target_).exists() || QFileInfo(target_).isSymLink())) {
                QMessageBox::warning(this, "Создать архив", "Архив уже существует: " + target_);
                return;
            }
            accept();
        });
        showDirectory(currentDirectory_);
        updatePreview();
        for (const QString &source : initialSources)
            addSource(QFileInfo(source).absoluteFilePath());
    }

    QString target() const { return target_; }
    QStringList sources() const { return selected_; }
    QString password() const {
        return encryption_ && encryption_->currentIndex() != 0 ? password_->text() : QString();
    }
    bool encryptHeaders() const { return encryption_ && encryption_->currentIndex() == 2; }

  private:
    void addSource(const QString &path) {
        if (!QFileInfo::exists(path) || selected_.contains(path))
            return;
        selected_ << path;
        const QFileInfo info(path);
        auto *item = new QListWidgetItem(info.fileName() + "\n" + info.absolutePath(), sources_);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
    }
    void showDirectory(const QString &path) {
        const QFileInfo info(path);
        if (!info.isDir()) {
            sourcePath_->setText(currentDirectory_);
            return;
        }
        currentDirectory_ = info.canonicalFilePath();
        sourcePath_->setText(currentDirectory_);
        available_->clear();
        for (const QFileInfo &entry :
             QDir(currentDirectory_)
                 .entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
                                QDir::DirsFirst | QDir::Name | QDir::IgnoreCase)) {
            auto *item =
                new QListWidgetItem((entry.isDir() ? "📁 " : "   ") + entry.fileName(), available_);
            item->setData(Qt::UserRole, entry.absoluteFilePath());
        }
    }

    QString currentDirectory_;
    QString target_;
    QString existingTarget_;
    QStringList selected_;
    QLineEdit *sourcePath_ = nullptr;
    QLineEdit *name_ = nullptr;
    QLineEdit *destination_ = nullptr;
    QLabel *preview_ = nullptr;
    QComboBox *format_ = nullptr;
    QComboBox *encryption_ = nullptr;
    QLineEdit *password_ = nullptr;
    QListWidget *available_ = nullptr;
    QListWidget *sources_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
};
