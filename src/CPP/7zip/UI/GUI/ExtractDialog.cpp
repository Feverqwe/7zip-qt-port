// Adapted from 7-Zip CPP/7zip/UI/GUI/ExtractDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "ExtractDialog.h"
#include "../../../Common/Wildcard.h"
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

static const int kPathModeButtonsVals[] = {
    NExtract::NPathMode::kFullPaths, NExtract::NPathMode::kNoPaths, NExtract::NPathMode::kAbsPaths};
static const int kOverwriteButtonsVals[] = {
    NExtract::NOverwriteMode::kAsk, NExtract::NOverwriteMode::kOverwrite,
    NExtract::NOverwriteMode::kSkip, NExtract::NOverwriteMode::kRename,
    NExtract::NOverwriteMode::kRenameExisting};
static const unsigned kHistorySize = 16;

static void AddComboItems(QComboBox &combo, const QStringList &labels, const int *values,
                          int curVal) {
    unsigned curSel = 0;
    for (unsigned i = 0; i < unsigned(labels.size()); i++) {
        QString s = labels[int(i)];
        s.remove('&');
        combo.addItem(s, i);
        if (values[i] == curVal)
            curSel = i;
    }
    combo.setCurrentIndex(int(curSel));
}

void AddUniqueString(CObjectVector<QString> &list, const QString &s) {
    FOR_VECTOR(i, list)
    if (s.compare(list[i], Qt::CaseInsensitive) == 0)
        return;
    list.Add(s);
}

static void NormalizeDirPathPrefix(QString &dirPath) {
    if (dirPath.isEmpty())
        return;
    bool isSepar = dirPath.back() == '/';
#ifdef Q_OS_WIN
    isSepar = isSepar || dirPath.back() == '\\';
#endif
    if (!isSepar)
        dirPath += '/';
}

CExtractDialog::CExtractDialog(const QString &initialPath, const QStringList &history,
                               bool selected, QWidget *parent)
    : QDialog(parent), DirPath(initialPath) {
    setObjectName("extractDialog");
    setWindowTitle(selected ? "Извлечь выбранное" : "Извлечь");
    resize(470, 330);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Извлечь в:", this));
    auto *pathRow = new QHBoxLayout;
    _path = new QComboBox(this);
    _path->setObjectName("extractDestination");
    _path->setEditable(true);
    _path->setInsertPolicy(QComboBox::NoInsert);

    pathRow->addWidget(_path, 1);
    auto *browse = new QPushButton("…", this);
    browse->setObjectName("extractBrowseButton");
    pathRow->addWidget(browse);
    layout->addLayout(pathRow);

    auto *options = new QHBoxLayout;
    auto *modeColumn = new QVBoxLayout;
    auto *nameRow = new QHBoxLayout;
    _nameEnable = new QCheckBox(this);
    _nameEnable->setObjectName("extractNameEnable");
    _nameEnable->setAccessibleName("Добавить имя папки назначения");
    _nameEnable->setToolTip("Добавить имя папки назначения");
    _pathName = new QLineEdit(this);
    _pathName->setObjectName("extractPathName");
    _pathName->setAccessibleName("Имя папки назначения");
    _pathName->setPlaceholderText("Имя папки назначения");
    nameRow->addWidget(_nameEnable);
    nameRow->addWidget(_pathName, 1);
    modeColumn->addLayout(nameRow);
    modeColumn->addWidget(new QLabel("Пути:", this));
    _pathMode = new QComboBox(this);
    _pathMode->setObjectName("extractPathMode");

    modeColumn->addWidget(_pathMode);
    modeColumn->addWidget(new QLabel("Перезапись:", this));
    _overwriteMode = new QComboBox(this);
    _overwriteMode->setObjectName("extractOverwriteMode");
    modeColumn->addWidget(_overwriteMode);
    options->addLayout(modeColumn, 1);

    auto *passwordGroup = new QGroupBox("Пароль", this);
    auto *passwordLayout = new QVBoxLayout(passwordGroup);
    _passwordControl = new QLineEdit(passwordGroup);
    _passwordControl->setObjectName("extractPassword");
    _passwordControl->setEchoMode(QLineEdit::Password);
    passwordLayout->addWidget(_passwordControl);
    _showPassword = new QCheckBox("Показать пароль", passwordGroup);
    _showPassword->setObjectName("extractShowPassword");
    passwordLayout->addWidget(_showPassword);
    options->addWidget(passwordGroup, 1, Qt::AlignTop);
    layout->addLayout(options);
    auto *hint = new QLabel("Папка назначения будет создана при необходимости. При выборе "
                            "«Без путей» файлы с одинаковыми именами из разных папок не "
                            "извлекаются.",
                            this);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    layout->addStretch();
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CExtractDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browse, &QPushButton::clicked, this, &CExtractDialog::OnButtonSetPath);
    connect(_showPassword, &QCheckBox::toggled, this, &CExtractDialog::UpdatePasswordControl);
    connect(_nameEnable, &QCheckBox::toggled, _pathName, &QWidget::setVisible);
    OnInit(history);
}

void CExtractDialog::SetInitialOptions(NExtract::NPathMode::EEnum pathMode,
                                       NExtract::NOverwriteMode::EEnum overwriteMode,
                                       const QString &password) {
    if (!PathMode_Force && _info.PathMode_Force)
        pathMode = _info.PathMode;
    if (!OverwriteMode_Force && _info.OverwriteMode_Force)
        overwriteMode = _info.OverwriteMode;
    PathMode = pathMode;
    OverwriteMode = overwriteMode;
    for (int index = 0; index < _pathMode->count(); ++index)
        if (kPathModeButtonsVals[_pathMode->itemData(index).toUInt()] ==
            (pathMode == NExtract::NPathMode::kCurPaths ? NExtract::NPathMode::kFullPaths
                                                        : pathMode))
            _pathMode->setCurrentIndex(index);
    for (int index = 0; index < _overwriteMode->count(); ++index)
        if (kOverwriteButtonsVals[_overwriteMode->itemData(index).toUInt()] == overwriteMode)
            _overwriteMode->setCurrentIndex(index);
    _passwordControl->setText(password);
}

QString CExtractDialog::settingsPath() {
    const QString overridden = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
    if (!overridden.isEmpty())
        return overridden;
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(directory);
    return QDir(directory).filePath("toolbar.ini");
}

void CExtractDialog::OnInit(const QStringList &history) {
    _info.Load(settingsPath());
    if (!history.isEmpty()) {
        _info.Paths.Clear();
        for (const QString &s : history)
            _info.Paths.Add(s);
    }
    if (_info.PathMode == NExtract::NPathMode::kCurPaths)
        _info.PathMode = NExtract::NPathMode::kFullPaths;
    if (!PathMode_Force && _info.PathMode_Force)
        PathMode = _info.PathMode;
    if (!OverwriteMode_Force && _info.OverwriteMode_Force)
        OverwriteMode = _info.OverwriteMode;
    _showPassword->setChecked(_info.ShowPassword.Val);
    UpdatePasswordControl();
    QString pathPrefix = DirPath;
    if (_info.SplitDest.Val) {
        _nameEnable->setChecked(true);
        QString pathName;
        SplitPathToParts_Smart(DirPath, pathPrefix, pathName);
        if (pathPrefix.isEmpty())
            pathPrefix = pathName;
        else
            _pathName->setText(pathName);
    } else
        _pathName->hide();
    _path->setEditText(pathPrefix);
    for (unsigned i = 0; i < _info.Paths.Size() && i < kHistorySize; i++)
        _path->addItem(_info.Paths[i]);
    // Qt addItem selects the first item in an editable empty combo. Restore typed text.
    _path->setCurrentIndex(-1);
    _path->setEditText(pathPrefix);
    AddComboItems(*_pathMode, {"Полные пути", "Без путей"}, kPathModeButtonsVals, PathMode);
    AddComboItems(*_overwriteMode,
                  {"Не перезаписывать существующие файлы", "Перезаписывать существующие файлы",
                   "Пропускать существующие файлы", "Переименовывать извлекаемые файлы",
                   "Переименовывать существующие файлы"},
                  kOverwriteButtonsVals, OverwriteMode);
}

void CExtractDialog::UpdatePasswordControl() {
    _passwordControl->setEchoMode(IsShowPasswordChecked() ? QLineEdit::Normal
                                                          : QLineEdit::Password);
}
void CExtractDialog::OnButtonSetPath() {
    const QString path =
        QFileDialog::getExistingDirectory(this, "Папка для извлечения", _path->currentText());
    if (!path.isEmpty())
        _path->setEditText(path);
}

void CExtractDialog::OnOK() {
    const int pathMode2 = kPathModeButtonsVals[_pathMode->currentData().toUInt()];
    if (PathMode != NExtract::NPathMode::kCurPaths || pathMode2 != NExtract::NPathMode::kFullPaths)
        PathMode = (NExtract::NPathMode::EEnum)pathMode2;
    OverwriteMode = (NExtract::NOverwriteMode::EEnum)
        kOverwriteButtonsVals[_overwriteMode->currentData().toUInt()];
    Password = _passwordControl->text();
    const bool showPassword = IsShowPasswordChecked();
    if (showPassword != _info.ShowPassword.Val) {
        _info.ShowPassword.Def = true;
        _info.ShowPassword.Val = showPassword;
    }
    if (_info.PathMode != pathMode2) {
        _info.PathMode_Force = true;
        _info.PathMode = (NExtract::NPathMode::EEnum)pathMode2;
    }
    if (!OverwriteMode_Force && _info.OverwriteMode != OverwriteMode)
        _info.OverwriteMode_Force = true;
    _info.OverwriteMode = OverwriteMode;
    QString s;
    int currentItem = _path->currentIndex();
    // A Qt editable combo may retain its index after setEditText: Win32 GetCurSel is CB_ERR.
    if (currentItem >= 0 && _path->currentText() != _path->itemText(currentItem))
        currentItem = -1;
    if (currentItem == -1) {
        s = _path->currentText();
        if (_path->count() >= int(kHistorySize))
            currentItem = _path->count() - 1;
    } else
        s = _path->itemText(currentItem);
    s = s.trimmed();
    NormalizeDirPathPrefix(s);
    const bool splitDest = _nameEnable->isChecked();
    if (splitDest) {
        QString pathName = _pathName->text().trimmed();
        s += pathName;
        NormalizeDirPathPrefix(s);
    }
    if (splitDest != _info.SplitDest.Val) {
        _info.SplitDest.Def = true;
        _info.SplitDest.Val = splitDest;
    }
    DirPath = s;
    _info.Paths.Clear();
    AddUniqueString(_info.Paths, s);
    for (int i = 0; i < _path->count(); i++)
        if (i != currentItem) {
            QString sTemp = _path->itemText(i).trimmed();
            AddUniqueString(_info.Paths, sTemp);
        }
    _info.Save(settingsPath());
    QDialog::accept();
}

QString CExtractDialog::destination() const {
    if (result() == QDialog::Accepted)
        return DirPath;
    QString s = _path->currentText().trimmed();
    if (_nameEnable->isChecked()) {
        NormalizeDirPathPrefix(s);
        s += _pathName->text().trimmed();
    }
    return s;
}

ExtractionOptions CExtractDialog::options() const {
    ExtractionOptions result;
    if (kPathModeButtonsVals[_pathMode->currentData().toUInt()] == NExtract::NPathMode::kNoPaths)
        result.pathMode = ExtractionOptions::PathMode::NoPaths;
    switch (kOverwriteButtonsVals[_overwriteMode->currentData().toUInt()]) {
    case NExtract::NOverwriteMode::kAsk:
        result.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        break;
    case NExtract::NOverwriteMode::kOverwrite:
        result.overwriteMode = ExtractionOptions::OverwriteMode::OverwriteExisting;
        break;
    case NExtract::NOverwriteMode::kSkip:
        result.overwriteMode = ExtractionOptions::OverwriteMode::SkipExisting;
        break;
    case NExtract::NOverwriteMode::kRename:
        result.overwriteMode = ExtractionOptions::OverwriteMode::RenameExtracted;
        break;
    case NExtract::NOverwriteMode::kRenameExisting:
        result.overwriteMode = ExtractionOptions::OverwriteMode::RenameExisting;
        break;
    default:
        break;
    }
    return result;
}
