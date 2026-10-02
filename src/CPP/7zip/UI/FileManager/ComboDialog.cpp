// Adapted from 7-Zip CPP/7zip/UI/FileManager/ComboDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt layouts replace Win32 OnSize/control attachment; QString/QStringList
// replace strings at the presentation boundary. OnOK keeps the raw value.
#include "ComboDialog.h"
#include "qtlangcontrols.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

CComboDialog::CComboDialog(const QString &title, const QString &prompt, const QString &initial,
                           QWidget *parent, const QStringList &strings)
    : QDialog(parent), Title(title), Static(prompt), Value(initial), Strings(strings) {
    OnInit();
}

bool CComboDialog::OnInit() {
    setObjectName("comboDialog");
    setWindowTitle(Title);
    resize(360, 150);
    auto *layout = new QVBoxLayout(this);
    auto *staticControl = new QLabel(Static, this);
    staticControl->setObjectName("comboPrompt");
    staticControl->setTextFormat(Qt::PlainText);
    staticControl->setWordWrap(true);
    layout->addWidget(staticControl);
    _comboBox = new QComboBox(this);
    _comboBox->setObjectName("comboValue");
    _comboBox->setAccessibleName(Static);
    _comboBox->setEditable(true);
    _comboBox->setInsertPolicy(QComboBox::NoInsert);
    _comboBox->setMinimumContentsLength(12);
    _comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    staticControl->setBuddy(_comboBox);
    // Native AddString leaves the edit text unchanged. Qt's first insertion
    // selects item 0, so restore the initial Value after populating Strings.
    _comboBox->setEditText(Value);
    for (const QString &s : Strings)
        _comboBox->addItem(s);
    _comboBox->setCurrentIndex(-1);
    _comboBox->setEditText(Value);
    layout->addWidget(_comboBox);
    layout->addStretch(1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("comboButtons");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CComboDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    _comboBox->setFocus();
    _comboBox->lineEdit()->selectAll();
    return true;
}

void CComboDialog::OnOK() {
    Value = _comboBox->currentText();
    QDialog::accept();
}

void CComboDialog::accept() { OnOK(); }

void CComboDialog::BindLanguage(UInt32 titleID, UInt32 promptID) {
    // A caller-built item title has no resource ID. Lang ID 0 names 7-Zip,
    // so binding it would replace the source "item : Comment" title.
    if (titleID)
        QtBindLangText(this, titleID, Title);
    QtBindLangText(findChild<QLabel *>("comboPrompt"), promptID, Static);
    QtBindLangText(_comboBox, promptID, Static, true);
}

QString CComboDialog::value() const {
    return result() == QDialog::Accepted ? Value : _comboBox->currentText();
}
