// Adapted from 7-Zip CPP/7zip/UI/FileManager/SplitDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt layouts/native folder browsing replace Win32 controls/OnSize; functional
// OnInit/OnOK/OnButtonSetPath retain upstream flow and public result members.
#include "SplitDialog.h"
#include "BrowseDialog.h"
#include "SplitUtils.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <limits>

CSplitDialog::CSplitDialog(const QString &filePath, const QString &path, QWidget *parent)
    : QDialog(parent), FilePath(filePath), Path(path) {
    OnInit();
}

bool CSplitDialog::OnInit() {
    setObjectName("splitDialog");
    setWindowTitle("Разбить файл");
    if (!FilePath.isEmpty())
        setWindowTitle(windowTitle() + ' ' + FilePath);
    resize(480, 240);
    auto *layout = new QVBoxLayout(this);
    auto *pathLabel = new QLabel("Разбить в:", this);
    pathLabel->setObjectName("splitPathLabel");
    layout->addWidget(pathLabel);
    auto *row = new QHBoxLayout;
    _pathCombo = new QComboBox(this);
    _pathCombo->setObjectName("volumeDestination");
    _pathCombo->setAccessibleName(pathLabel->text());
    _pathCombo->setEditable(true);
    _pathCombo->setInsertPolicy(QComboBox::NoInsert);
    _pathCombo->setMinimumContentsLength(12);
    _pathCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    pathLabel->setBuddy(_pathCombo);
    row->addWidget(_pathCombo, 1);
    auto *browse = new QPushButton("…", this);
    browse->setObjectName("volumeBrowse");
    browse->setAccessibleName("Папка назначения");
    row->addWidget(browse);
    layout->addLayout(row);
    auto *volumeLabel = new QLabel("Размер тома, байт:", this);
    volumeLabel->setObjectName("splitVolumeLabel");
    layout->addWidget(volumeLabel);
    _volumeCombo = new QComboBox(this);
    _volumeCombo->setObjectName("volumeSize");
    _volumeCombo->setAccessibleName(volumeLabel->text());
    _volumeCombo->setEditable(true);
    _volumeCombo->setInsertPolicy(QComboBox::NoInsert);
    volumeLabel->setBuddy(_volumeCombo);
    layout->addWidget(_volumeCombo);
    error_ = new QLabel(this);
    error_->setObjectName("volumeError");
    error_->setWordWrap(true);
    error_->hide();
    layout->addWidget(error_);
    layout->addStretch(1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("splitButtons");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CSplitDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browse, &QPushButton::clicked, this, &CSplitDialog::OnButtonSetPath);

    _pathCombo->setCurrentText(Path);
    AddVolumeItems(*_volumeCombo);
    _volumeCombo->setCurrentIndex(0);
    _pathCombo->setFocus();
    _pathCombo->lineEdit()->selectAll();
    return true;
}

void CSplitDialog::OnButtonSetPath() {
    const QString currentPath = _pathCombo->currentText();
    QString resultPath;
    if (!MyBrowseForFolder(this, "Папка назначения", currentPath, resultPath))
        return;
    resultPath = QDir::fromNativeSeparators(resultPath);
    if (!resultPath.endsWith('/'))
        resultPath += '/';
    _pathCombo->setCurrentIndex(-1);
    _pathCombo->setCurrentText(resultPath);
}

void CSplitDialog::OnOK() {
    Path = _pathCombo->currentText();
    const QString volumeString = _volumeCombo->currentText().trimmed();
    CRecordVector<UInt64> values;
    VolumeSizes.Clear();
    bool valid =
        ParseVolumeSizes(UString(volumeString.toStdWString().c_str()), values) && !values.IsEmpty();
    FOR_VECTOR(i, values) {
        const UInt64 value = values[i];
        // The Qt worker/file API has signed 64-bit offsets.
        if (value > UInt64(std::numeric_limits<qint64>::max()))
            valid = false;
        VolumeSizes.Add(value);
    }
    if (!valid) {
        error_->setText("Укажите положительные размеры томов (например, 10M или 10M 20M).");
        error_->show();
        return;
    }
    error_->clear();
    error_->hide();
    QDialog::accept();
}

void CSplitDialog::accept() { OnOK(); }
