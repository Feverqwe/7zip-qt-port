#include "createarchivedialog.h"

#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include <QScrollArea>

CreateArchiveDialog::CreateArchiveDialog(const QString &directory,
                                         const QStringList &initialSources, QWidget *parent,
                                         const QString &existingTarget)
    : QDialog(parent), currentDirectory_(directory), StartDirPrefix(directory),
      existingTarget_(existingTarget) {
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
    m_RegistryInfo.Load(compressionSettingsPath());
    createArchiveForm(contentLayout);
    createSourcePicker(contentLayout);
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
    archivePath_ = new QComboBox(this);
    archivePath_->setObjectName("archivePath");
    archivePath_->setEditable(true);
    archivePath_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    archivePath_->setMinimumContentsLength(20);
    archivePath_->setInsertPolicy(QComboBox::NoInsert);
    for (unsigned i = 0; i < m_RegistryInfo.ArcPaths.Size() && i < 20; i++)
        archivePath_->addItem(m_RegistryInfo.ArcPaths[i]);
    name_ = archivePath_->lineEdit();
    name_->setObjectName("archiveName");
    form->addRow("Архив:", archivePath_);
    format_ = new QComboBox(this);
    format_->setObjectName("archiveFormat");
    format_->addItems({"7z", "zip"});
    const QString initialFormat = existingTarget_.isEmpty()
                                      ? m_RegistryInfo.ArcType
                                      : QFileInfo(existingTarget_).suffix().toLower();
    const int savedFormat = format_->findText(initialFormat, Qt::MatchFixedString);
    format_->setCurrentIndex(savedFormat >= 0 ? savedFormat : 0);
    CCompressDialog path;
    path.Format = format_->currentText();
    path.SetArchiveName("archive");
    name_->setText(path.m_ArchivePath);
    form->addRow("Формат:", format_);
    if (!existingTarget_.isEmpty()) {
        updateMode_ = new QComboBox(this);
        updateMode_->setObjectName("archiveUpdateMode");
        updateMode_->addItem("Добавить и заменить", int(NCompressDialog::NUpdateMode::kAdd));
        updateMode_->addItem("Обновить и добавить", int(NCompressDialog::NUpdateMode::kUpdate));
        updateMode_->addItem("Обновить существующие", int(NCompressDialog::NUpdateMode::kFresh));
        updateMode_->addItem("Синхронизировать", int(NCompressDialog::NUpdateMode::kSync));
        form->addRow("Режим обновления:", updateMode_);
    }
    pathMode_ = new QComboBox(this);
    pathMode_->setObjectName("archivePathMode");
    pathMode_->addItem("Относительные пути", int(NWildcard::k_RelatPath));
    pathMode_->addItem("Полные пути", int(NWildcard::k_FullPath));
    pathMode_->addItem("Абсолютные пути", int(NWildcard::k_AbsPath));
    form->addRow("Режим путей:", pathMode_);
    {
        // CompressDialog.rc places compression on the left and encryption on
        // the right. Keep both columns at compact sizes; scrolling is a Qt
        // adaptation for the additional source picker below the options.
        auto *columns = new QHBoxLayout;
        auto *compressionColumn = new QWidget(this);
        compressionColumn->setObjectName("archiveCompressionColumn");
        auto *compressionForm = new QFormLayout(compressionColumn);
        compressionForm->setContentsMargins(0, 0, 0, 0);
        auto *encryptionColumn = new QWidget(this);
        encryptionColumn->setObjectName("archiveEncryptionColumn");
        auto *encryptionForm = new QFormLayout(encryptionColumn);
        encryptionForm->setContentsMargins(0, 0, 0, 0);
        createCompressionOptions(compressionForm);
        createLifecycleOptions(encryptionForm);
        if (existingTarget_.isEmpty())
            createEncryptionOptions(encryptionForm);
        columns->addWidget(compressionColumn, 1, Qt::AlignTop);
        columns->addWidget(encryptionColumn, 1, Qt::AlignTop);
        form->addRow(columns);
    }
    createVolumeOptions(form);
    createAdvancedOptions(form);
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
        name_->setText(target.fileName());
        format_->setCurrentText(target.suffix().toLower());
        destination_->setText(target.absolutePath());
        archivePath_->setEnabled(false);
        format_->setEnabled(false);
        destination_->setEnabled(false);
        browse->setEnabled(false);
    }
    connect(browse, &QPushButton::clicked, this, [this] {
        QString path;
        if (MyBrowseForFolder(this, "Место сохранения", destination_->text(), path))
            destination_->setText(path);
    });
    connect(format_, &QComboBox::currentTextChanged, this, [this] {
        CCompressDialog path;
        path.Format = format_->currentText();
        path.m_ArchivePath = name_->text();
        path.SetArchiveName2();
        name_->setText(path.m_ArchivePath);
    });
    connect(archivePath_, &QComboBox::activated, this, [this](int index) {
        CCompressDialog path;
        const QString selectedPath = archivePath_->itemText(index);
        path.Format = format_->currentText();
        path.ArcPath_WasChanged(selectedPath);
        format_->setCurrentText(path.Format);
        path.SetArcPathFields(selectedPath);
        destination_->setText(path.DirPrefix);
        name_->setText(path.m_ArchivePath);
    });
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
    preview_->setText(existingTarget_.isEmpty() ? "Новый архив: " + archivePath()
                                                : "Добавить в открытый архив: " + existingTarget_);
}

QString CreateArchiveDialog::archivePath() const {
    CCompressDialog path;
    path.DirPrefix = destination_->text();
    path.StartDirPrefix = StartDirPrefix;
    path.m_ArchivePath = name_->text();
    QString result;
    path.GetFinalPath_Smart(result);
    return result;
}

void CreateArchiveDialog::validateAndAccept() {
    if (password_) {
        const auto error = CCompressDialog::CheckPassword(
            password_->text(), passwordConfirmation_->text(), showPassword_->isChecked(),
            format_->currentText(), GetEncryptionMethodSpec());
        if (error != CCompressDialog::kPasswordOK) {
            const QString message =
                error == CCompressDialog::kPasswordUseAscii
                    ? "Для ZIP используйте пароль из ASCII без управляющих символов."
                : error == CCompressDialog::kPasswordTooLong
                    ? "Пароль ZIP AES не должен превышать 99 символов."
                    : "Введённые пароли не совпадают.";
            QMessageBox::warning(this, "Создать архив", message);
            return;
        }
    }
    if (!checkMemoryUsage())
        return;
    if (!checkVolumeSizes())
        return;
    const QString base = name_->text().trimmed();
    const QFileInfo destination(QFileInfo(archivePath()).absolutePath());
    if (selected_.isEmpty() ||
        (existingTarget_.isEmpty() &&
         (base.isEmpty() || archivePath().isEmpty() || !destination.isDir() ||
          QFileInfo(archivePath()).suffix().compare(format_->currentText(), Qt::CaseInsensitive) !=
              0))) {
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
    target_ = existingTarget_.isEmpty() ? archivePath() : existingTarget_;
    if (existingTarget_.isEmpty() &&
        (QFileInfo(target_).exists() || QFileInfo(target_).isSymLink())) {
        QMessageBox::warning(this, "Создать архив", "Архив уже существует: " + target_);
        return;
    }
    SaveOptionsInMem();
    CCompressDialog path;
    path.m_ArchivePath = target_;
    path.Format = format_->currentText();
    path.ShowPassword = showPassword_ && showPassword_->isChecked();
    path.EncryptHeaders = encryptHeaders_ && encryptHeaders_->isChecked();
    path.OnOK(m_RegistryInfo);
    m_RegistryInfo.Save(compressionSettingsPath());
    accept();
}

CompressionOptions CreateArchiveDialog::compressionOptions() const {
    CompressionOptions options;
    options.OpenShareForWrite =
        sharedFiles_ && sharedFiles_->isEnabled() && sharedFiles_->isChecked();
    options.DeleteAfterCompressing = deleteAfterCompressing_ &&
                                     deleteAfterCompressing_->isEnabled() &&
                                     deleteAfterCompressing_->isChecked();
#ifdef Q_OS_WIN
    options.PreserveATime = m_RegistryInfo.PreserveATime;
#endif
    if (parameters_) {
        options.Options = parameters_->text().toStdWString().c_str();
        options.Options.Trim();
    }
    const int index = FindRegistryFormat(registryFormat_);
    if (index >= 0) {
        const auto &fo = m_RegistryInfo.Formats[index];
        options.TimePrec = fo.TimePrec;
        options.MTime = fo.MTime;
        options.CTime = fo.CTime;
        options.ATime = fo.ATime;
        options.SetArcMTime = fo.SetArcMTime;
    }
    if (volume_ && volume_->isEnabled())
        CCompressDialog::OnOK(volume_->currentText(), options);
    if (updateMode_)
        options.UpdateMode =
            static_cast<NCompressDialog::NUpdateMode::EEnum>(updateMode_->currentData().toInt());
    options.PathMode = static_cast<NWildcard::ECensorPathMode>(pathMode_->currentData().toInt());
    if (level_) {
        if (encryption_)
            options.EncryptionMethod = GetEncryptionMethodSpec();
        options.level = level_->currentData().toInt();
        options.method = method_->currentText();
        options.threads = threads_->currentData().toInt();
        options.MemUsage = Get_MemUse_Spec();
        options.dictionaryBytes =
            dictionary_->isEnabled() ? dictionary_->currentData().toULongLong() : 0;
        options.wordSize = wordSize_->isEnabled() ? wordSize_->currentData().toInt() : 0;
        options.SolidIsSpecified = solidBlock_->isEnabled();
        NCompressDialog::OnOK(
            solidBlock_->isEnabled() ? solidBlock_->currentData().toUInt() : UInt32(-1), options);
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
