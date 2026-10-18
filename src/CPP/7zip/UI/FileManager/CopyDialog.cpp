// Adapted from 7-Zip CPP/7zip/UI/FileManager/CopyDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt layouts replace Win32 OnSize/control attachment. Folder browsing uses
// the native Qt dialog; the upstream normalization and selection reset remain.
#include "CopyDialog.h"
#include "7zip/UI/FileManager/CopyDialogRes.h"
#include "7zip/UI/FileManager/resource.h"
#include "BrowseDialog.h"
#include "ViewSettings.h"
#include "Windows/FileName.h"
#include "qtdialogscreenservices.h"
#include "qtdialogunits.h"
#include "qtlangcontrols.h"

#include "qtlangbuttonbox.h"
#include <QComboBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextDocument>
#include <QVBoxLayout>

CCopyDialog::CCopyDialog(const QString &title, const QString &label, const QString &path,
                         const QString &info, QWidget *parent)
    : QDialog(parent), Title(ToUString(title)), Static(ToUString(label)), Value(ToUString(path)),
      Info(ToUString(info)) {
    OnInit();
}

CCopyDialog::CCopyDialog(bool move, const QString &initialPath, const QStringList &sources,
                         const QStringList &history, QWidget *parent, const QString &info)
    : QDialog(parent), Title(ToUString(QtLangText(move ? IDS_MOVE : IDS_COPY))),
      Static(ToUString(QtLangText(move ? IDS_MOVE_TO : IDS_COPY_TO))),
      Value(ToUString(initialPath)), Info(ToUString(info)) {
    for (const QString &s : history)
        Strings.Add(ToUString(s));
    // The Qt caller supplies filenames, rather than IFolder property indices.
    if (info.isNull())
        for (const QString &source : sources) {
            if (!Info.IsEmpty())
                Info.Add_LF();
            Info += ToUString(QFileInfo(source).fileName());
        }
    OnInit();
    BindLanguage(move ? IDS_MOVE : IDS_COPY, move ? IDS_MOVE_TO : IDS_COPY_TO);
}

void CCopyDialog::BindLanguage(UInt32 titleID, UInt32 promptID, const QString &titleSuffix) {
    setProperty("languageSuffix", titleSuffix);
    QtBindLangText(this, titleID, ToQString(Title));
    QtBindLangText(findChild<QLabel *>("copyDestinationLabel"), promptID, ToQString(Static));
    QtBindLangText(_path, promptID, ToQString(Static), true);
}

bool CCopyDialog::OnInit() {
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setObjectName("copyDialog");
    setWindowTitle(ToQString(Title));
    // CopyDialog.rc: xc=320, yc=144; GuiCommon.rc adds m=8 on each side.
    resize(QtDialogUnits(font()).pixels(QSize(336, 160)));
    auto *layout = new QVBoxLayout(this);
    const QSize margin = QtDialogUnits(font()).pixels(QSize(8, 8));
    layout->setContentsMargins(margin.width(), margin.height(), margin.width(), margin.height());
    layout->setSpacing(QtDialogUnits(font()).pixels(QSize(0, 4)).height());
    auto *label = new QLabel(ToQString(Static), this);
    label->setObjectName("copyDestinationLabel");
    layout->addWidget(label);
    auto *pathRow = new QHBoxLayout;
    _path = new QComboBox(this);
    _path->setObjectName("copyDestination");
    _path->setAccessibleName(ToQString(Static));
    _path->setEditable(true);
    _path->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    _path->setInsertPolicy(QComboBox::NoInsert);
    FOR_VECTOR(i, Strings)
    _path->addItem(ToQString(Strings[i]));
    _path->setCurrentText(ToQString(Value));
    label->setBuddy(_path);
    _path->setMinimumContentsLength(12);
    _path->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    pathRow->addWidget(_path, 1);
    auto *browse = new QPushButton("…", this);
    browse->setObjectName("copyBrowseButton");
    browse->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    QtBindLangText(browse, IDS_SET_FOLDER, "Select folder", true);
    browse->setFixedWidth(QtDialogUnits(font()).pixels(QSize(20, 0)).width());
    pathRow->addWidget(browse);
    layout->addLayout(pathRow);
    // Native SS_LEFTNOWORDWRAP shows a bounded summary. Qt scrolling keeps long
    // Unicode filenames and large selections reachable at compact sizes.
    auto *info = new QPlainTextEdit(ToQString(Info), this);
    info->setObjectName("copySelectionInfo");
    QtBindLangText(info, IDS_PROPERTIES, "Properties", true);
    info->setReadOnly(true);
    info->document()->setDocumentMargin(0);
    // Native IDT_COPY_INFO is static, not a separate keyboard tab stop.
    info->setFocusPolicy(Qt::NoFocus);
    info->setFrameShape(QFrame::NoFrame);
    info->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(info, 1);
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("copyButtons");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CCopyDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browse, &QPushButton::clicked, this, &CCopyDialog::OnButtonSetPath);
    QtNormalizeDialogSize(*this, true);
    _path->setFocus();
    _path->lineEdit()->selectAll();
    return true;
}

void CCopyDialog::OnButtonSetPath() {
    const QString currentPath = _path->currentText();
    UString selectedPath;
    if (!MyBrowseForFolder(this, ToUString(QtLangText(IDS_SET_FOLDER)), ToUString(currentPath),
                           selectedPath))
        return;
    NWindows::NFile::NName::NormalizeDirPathPrefix(selectedPath);
    _path->setCurrentIndex(-1);
    _path->setCurrentText(ToQString(selectedPath));
}

void CCopyDialog::OnOK() {
    Value = ToUString(_path->currentText());
    QDialog::accept();
}

void CCopyDialog::accept() { OnOK(); }

QString CCopyDialog::destination() const {
    return result() == QDialog::Accepted ? ToQString(Value) : _path->currentText();
}
