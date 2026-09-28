// Adapted from 7-Zip CPP/7zip/UI/FileManager/CopyDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt layouts replace Win32 OnSize/control attachment. Folder browsing uses
// the native Qt dialog; the upstream normalization and selection reset remain.
#include "CopyDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

CCopyDialog::CCopyDialog(const QString &title, const QString &label, const QString &path,
                         const QString &info, QWidget *parent)
    : QDialog(parent), Title(title), Static(label), Value(path), Info(info) {
    OnInit();
}

CCopyDialog::CCopyDialog(bool move, const QString &initialPath, const QStringList &sources,
                         const QStringList &history, QWidget *parent, const QString &info)
    : QDialog(parent), Title(move ? "Переместить" : "Копировать"),
      Static(move ? "Переместить в:" : "Копировать в:"), Value(initialPath), Info(info),
      Strings(history) {
    // The Qt caller supplies filenames, rather than IFolder property indices.
    if (Info.isNull())
        for (const QString &source : sources) {
            if (!Info.isEmpty())
                Info += '\n';
            Info += QFileInfo(source).fileName();
        }
    OnInit();
}

bool CCopyDialog::OnInit() {
    setObjectName("copyDialog");
    setWindowTitle(Title);
    resize(440, 250);
    auto *layout = new QVBoxLayout(this);
    auto *label = new QLabel(Static, this);
    label->setObjectName("copyDestinationLabel");
    layout->addWidget(label);
    auto *pathRow = new QHBoxLayout;
    _path = new QComboBox(this);
    _path->setObjectName("copyDestination");
    _path->setAccessibleName(Static);
    _path->setEditable(true);
    _path->setInsertPolicy(QComboBox::NoInsert);
    for (const QString &s : Strings)
        _path->addItem(s);
    _path->setCurrentText(Value);
    label->setBuddy(_path);
    _path->setMinimumContentsLength(12);
    _path->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    pathRow->addWidget(_path, 1);
    auto *browse = new QPushButton("…", this);
    browse->setObjectName("copyBrowseButton");
    browse->setAccessibleName("Папка назначения");
    pathRow->addWidget(browse);
    layout->addLayout(pathRow);
    // Native SS_LEFTNOWORDWRAP shows a bounded summary. Qt scrolling keeps long
    // Unicode filenames and large selections reachable at compact sizes.
    auto *info = new QPlainTextEdit(Info, this);
    info->setObjectName("copySelectionInfo");
    info->setAccessibleName("Выбранные объекты");
    info->setReadOnly(true);
    info->setFrameShape(QFrame::NoFrame);
    info->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(info, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("copyButtons");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CCopyDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browse, &QPushButton::clicked, this, &CCopyDialog::OnButtonSetPath);
    _path->setFocus();
    _path->lineEdit()->selectAll();
    return true;
}

void CCopyDialog::OnButtonSetPath() {
    const QString currentPath = _path->currentText();
    QString resultPath = QFileDialog::getExistingDirectory(this, "Папка назначения", currentPath);
    if (resultPath.isEmpty())
        return;
    resultPath = QDir::fromNativeSeparators(resultPath);
    if (!resultPath.endsWith('/'))
        resultPath += '/';
    _path->setCurrentIndex(-1);
    _path->setCurrentText(resultPath);
}

void CCopyDialog::OnOK() {
    Value = _path->currentText();
    QDialog::accept();
}

void CCopyDialog::accept() { OnOK(); }

QString CCopyDialog::destination() const {
    return result() == QDialog::Accepted ? Value : _path->currentText();
}
