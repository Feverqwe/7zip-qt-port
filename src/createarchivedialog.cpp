#include "createarchivedialog.h"

#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include <QScrollArea>

CreateArchiveDialog::CreateArchiveDialog(const QString &directory,
                                         const QStringList &initialSources, QWidget *parent,
                                         const QString &existingTarget)
    : QDialog(parent), currentDirectory_(directory), existingTarget_(existingTarget) {
    setObjectName("createArchiveDialog");
    setWindowTitle(existingTarget_.isEmpty() ? "Создать архив" : "Добавить в архив");
    resize(760, 720);
    auto *layout = new QVBoxLayout(this);
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("archiveOptionsScrollArea");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    createSourcePicker(contentLayout);
    createArchiveForm(contentLayout);
    scroll->setWidget(content);
    layout->addWidget(scroll);
    createFooter(layout);
    showDirectory(currentDirectory_);
    updateTargetPreview();
    for (const QString &source : initialSources)
        addSource(QFileInfo(source).absoluteFilePath());
}

void CreateArchiveDialog::createSourcePicker(QVBoxLayout *layout) {
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
}

void CreateArchiveDialog::createArchiveForm(QVBoxLayout *layout) {
    auto *form = new QFormLayout;
    name_ = new QLineEdit("archive", this);
    name_->setObjectName("archiveName");
    form->addRow("Имя:", name_);
    format_ = new QComboBox(this);
    format_->setObjectName("archiveFormat");
    format_->addItems({"7z", "zip"});
    form->addRow("Формат:", format_);
    if (existingTarget_.isEmpty()) {
        createCompressionOptions(form);
        createEncryptionOptions(form);
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
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString path =
            QFileDialog::getExistingDirectory(this, "Место сохранения", destination_->text());
        if (!path.isEmpty())
            destination_->setText(path);
    });
}

void CreateArchiveDialog::createCompressionOptions(QFormLayout *form) {
    auto *compression = new QGroupBox("Параметры сжатия", this);
    auto *compressionForm = new QFormLayout(compression);
    level_ = new QComboBox(compression);
    level_->setObjectName("compressionLevel");
    compressionForm->addRow("Уровень сжатия:", level_);
    method_ = new QComboBox(compression);
    method_->setObjectName("compressionMethod");
    compressionForm->addRow("Метод сжатия:", method_);
    dictionary_ = new QComboBox(compression);
    dictionary_->setObjectName("compressionDictionary");
    dictionary_->addItem("Автоматически", 0);
    for (int size : {1, 4, 16, 64})
        dictionary_->addItem(QString::number(size) + " МиБ", size);
    compressionForm->addRow("Размер словаря:", dictionary_);
    wordSize_ = new QComboBox(compression);
    wordSize_->setObjectName("compressionWordSize");
    wordSize_->addItem("Автоматически", 0);
    for (int size : {16, 32, 64, 128, 273})
        wordSize_->addItem(QString::number(size), size);
    compressionForm->addRow("Размер слова:", wordSize_);
    solid_ = new QCheckBox("Непрерывный архив", compression);
    solid_->setObjectName("solidArchiveOption");
    solid_->setChecked(true);
    compressionForm->addRow(solid_);
    solidBlock_ = new QComboBox(compression);
    solidBlock_->setObjectName("compressionSolidBlock");
    solidBlock_->addItem("Автоматически", 0);
    for (int size : {16, 64, 256})
        solidBlock_->addItem(QString::number(size) + " МиБ", size);
    compressionForm->addRow("Размер непрерывного блока:", solidBlock_);
    threads_ = new QSpinBox(compression);
    threads_->setObjectName("compressionThreads");
    threads_->setRange(0, 256);
    threads_->setSpecialValueText("Автоматически");
    compressionForm->addRow("Потоки CPU:", threads_);
    form->addRow(compression);
    connect(format_, &QComboBox::currentTextChanged, this, [this] {
        method_->clear();
        SetLevel2();
        method_->addItems(NCompressDialog::GetMethodNames(format_->currentText()));
        solid_->setEnabled(format_->currentText() == "7z");
        solidBlock_->setEnabled(format_->currentText() == "7z" && solid_->isChecked() &&
                                level_->currentData().toInt() != 0);
    });
    const auto updateMethodOptions = [this] {
        const bool lzma = method_->currentText() == "LZMA" || method_->currentText() == "LZMA2";
        const bool compressed = level_->currentData().toInt() != 0;
        method_->setEnabled(compressed);
        dictionary_->setEnabled(compressed && lzma);
        wordSize_->setEnabled(compressed && lzma);
        solidBlock_->setEnabled(compressed && format_->currentText() == "7z" &&
                                solid_->isChecked());
    };
    connect(level_, &QComboBox::currentIndexChanged, this, updateMethodOptions);
    connect(method_, &QComboBox::currentTextChanged, this, updateMethodOptions);
    connect(solid_, &QCheckBox::toggled, this, [this](bool enabled) {
        solidBlock_->setEnabled(format_->currentText() == "7z" && enabled &&
                                level_->currentData().toInt() != 0);
    });
    SetLevel2();
    method_->addItems(NCompressDialog::GetMethodNames(format_->currentText()));
}

// Adapted from CCompressDialog::SetLevel2 / SetNearestSelectComboBox (0766b73).
// Registry format defaults are replaced by the current Qt selection (initially 5).
void CreateArchiveDialog::SetLevel2() {
    const UInt32 level = level_->currentIndex() < 0 ? 5 : level_->currentData().toUInt();
    level_->clear();
    const auto &fi = NCompressDialog::GetFormatInfo(format_->currentText());
    const QString labels[] = {"Без сжатия", "Быстрое",      {}, "Быстро", {}, "Обычное",
                              {},           "Максимальное", {}, "Ультра"};
    for (unsigned i = 0; i < sizeof(UInt32) * 8; i++) {
        const UInt32 mask = fi.LevelsMask >> i;
        if (mask & 1) {
            QString text = QString::number(i);
            if (i < Z7_ARRAY_SIZE(labels) && !labels[i].isEmpty())
                text += " - " + labels[i];
            level_->addItem(text, i);
        }
    }
    for (int i = level_->count() - 1; i >= 0; i--)
        if (level_->itemData(i).toUInt() <= level) {
            level_->setCurrentIndex(i);
            return;
        }
    if (level_->count() > 0)
        level_->setCurrentIndex(0);
}

void CreateArchiveDialog::createEncryptionOptions(QFormLayout *form) {
    auto *encryptionGroup = new QGroupBox("Шифрование", this);
    auto *encryptionForm = new QFormLayout(encryptionGroup);
    encryption_ = new QComboBox(this);
    encryption_->setObjectName("archiveEncryption");
    encryptionForm->addRow("Метод:", encryption_);
    password_ = new QLineEdit(this);
    password_->setObjectName("archivePassword");
    password_->setEchoMode(QLineEdit::Password);
    encryptionForm->addRow("Пароль:", password_);
    passwordConfirmation_ = new QLineEdit(this);
    passwordConfirmation_->setObjectName("archivePasswordConfirmation");
    passwordConfirmation_->setEchoMode(QLineEdit::Password);
    encryptionForm->addRow("Повторите пароль:", passwordConfirmation_);
    auto *showPassword = new QCheckBox("Показать пароль", encryptionGroup);
    showPassword->setObjectName("archiveShowPassword");
    encryptionForm->addRow(showPassword);
    form->addRow(encryptionGroup);
    connect(showPassword, &QCheckBox::toggled, this, [this](bool visible) {
        const auto mode = visible ? QLineEdit::Normal : QLineEdit::Password;
        password_->setEchoMode(mode);
        passwordConfirmation_->setEchoMode(mode);
    });
    const auto updateEncryption = [this] {
        encryption_->clear();
        encryption_->addItem("Без шифрования");
        encryption_->addItem("AES-256");
        if (format_->currentText() == "7z")
            encryption_->addItem("AES-256 и шифрование имён файлов");
    };
    connect(format_, &QComboBox::currentTextChanged, this, updateEncryption);
    connect(encryption_, &QComboBox::currentIndexChanged, this, [this] {
        password_->setEnabled(encryption_->currentIndex() != 0);
        passwordConfirmation_->setEnabled(encryption_->currentIndex() != 0);
    });
    updateEncryption();
    password_->setEnabled(false);
    passwordConfirmation_->setEnabled(false);
}

void CreateArchiveDialog::createFooter(QVBoxLayout *layout) {
    preview_ = new QLabel(this);
    preview_->setObjectName("archiveTargetPreview");
    preview_->setWordWrap(true);
    layout->addWidget(preview_);
    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons_->button(QDialogButtonBox::Ok)
        ->setText(existingTarget_.isEmpty() ? "Создать" : "Добавить");
    buttons_->button(QDialogButtonBox::Cancel)->setText("Отмена");
    layout->addWidget(buttons_);

    connect(name_, &QLineEdit::textChanged, this, &CreateArchiveDialog::updateTargetPreview);
    connect(destination_, &QLineEdit::textChanged, this, &CreateArchiveDialog::updateTargetPreview);
    connect(format_, &QComboBox::currentTextChanged, this,
            &CreateArchiveDialog::updateTargetPreview);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons_, &QDialogButtonBox::accepted, this, &CreateArchiveDialog::validateAndAccept);
}

void CreateArchiveDialog::updateTargetPreview() {
    preview_->setText(existingTarget_.isEmpty()
                          ? "Новый архив: " + QDir(destination_->text())
                                                  .filePath(name_->text().trimmed() + "." +
                                                            format_->currentText())
                          : "Добавить в открытый архив: " + existingTarget_);
}

void CreateArchiveDialog::validateAndAccept() {
    if (encryption_ && encryption_->currentIndex() != 0 && password_->text().isEmpty()) {
        QMessageBox::warning(this, "Создать архив", "Введите пароль для шифрования.");
        return;
    }
    if (encryption_ && encryption_->currentIndex() != 0 &&
        password_->text() != passwordConfirmation_->text()) {
        QMessageBox::warning(this, "Создать архив", "Введённые пароли не совпадают.");
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
    target_ =
        existingTarget_.isEmpty()
            ? QDir(destination.absoluteFilePath()).filePath(base + "." + format_->currentText())
            : existingTarget_;
    if (existingTarget_.isEmpty() &&
        (QFileInfo(target_).exists() || QFileInfo(target_).isSymLink())) {
        QMessageBox::warning(this, "Создать архив", "Архив уже существует: " + target_);
        return;
    }
    accept();
}

CompressionOptions CreateArchiveDialog::compressionOptions() const {
    CompressionOptions options;
    if (level_) {
        options.level = level_->currentData().toInt();
        options.method = method_->currentText();
        options.threads = threads_->value();
        options.solid = solid_->isChecked();
        options.dictionaryMiB = dictionary_->isEnabled() ? dictionary_->currentData().toInt() : 0;
        options.wordSize = wordSize_->isEnabled() ? wordSize_->currentData().toInt() : 0;
        options.solidBlockMiB = solidBlock_->isEnabled() ? solidBlock_->currentData().toInt() : 0;
    }
    return options;
}

void CreateArchiveDialog::addSource(const QString &path) {
    if (!QFileInfo::exists(path) || selected_.contains(path))
        return;
    selected_ << path;
    const QFileInfo info(path);
    auto *item = new QListWidgetItem(info.fileName() + "\n" + info.absolutePath(), sources_);
    item->setData(Qt::UserRole, path);
    item->setToolTip(path);
}

void CreateArchiveDialog::showDirectory(const QString &path) {
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
