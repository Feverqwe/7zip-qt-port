// Adapted from 7-Zip CPP/7zip/UI/FileManager/PasswordDialog.cpp/.rc (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt controls replace Win32 attachment/messages; functional handler flow stays.
#include "PasswordDialog.h"
#include "7zip/UI/FileManager/PasswordDialogRes.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

CPasswordDialog::CPasswordDialog(QWidget *parent) : QDialog(parent) {}

void CPasswordDialog::ReadControls() {
    Password = _passwordEdit->text().toStdWString().c_str();
    ShowPassword = _showPassword->isChecked();
}

void CPasswordDialog::SetTextSpec() {
    _passwordEdit->setEchoMode(ShowPassword ? QLineEdit::Normal : QLineEdit::Password);
    _passwordEdit->setText(QString::fromWCharArray(Password.Ptr(), Password.Len()));
}

bool CPasswordDialog::OnInit() {
    setObjectName("passwordDialog");
    QtBindResourceText(this, IDD_PASSWORD, "Enter password");
    resize(360, 170);
    auto *layout = new QVBoxLayout(this);
    auto *label = new QLabel(this);
    QtBindResourceText(label, IDT_PASSWORD_ENTER, "&Enter password:");
    label->setObjectName("passwordPrompt");
    layout->addWidget(label);
    _passwordEdit = new QLineEdit(this);
    _passwordEdit->setObjectName("passwordEdit");
    QtBindLangText(_passwordEdit, IDT_PASSWORD_ENTER, "Enter password:", true);
    label->setBuddy(_passwordEdit);
    layout->addWidget(_passwordEdit);
    _showPassword = new QCheckBox(this);
    QtBindResourceText(_showPassword, IDX_PASSWORD_SHOW, "&Show password");
    _showPassword->setObjectName("passwordShow");
    layout->addWidget(_showPassword);
    _showPassword->setChecked(ShowPassword);
    SetTextSpec();
    layout->addStretch(1);
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("passwordButtons");
    layout->addWidget(buttons);
    connect(_showPassword, &QCheckBox::clicked, this, &CPasswordDialog::OnButtonClicked);
    connect(buttons, &QDialogButtonBox::accepted, this, &CPasswordDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    _passwordEdit->setFocus();
    return true;
}

void CPasswordDialog::OnButtonClicked() {
    ReadControls();
    SetTextSpec();
}

void CPasswordDialog::OnOK() {
    ReadControls();
    QDialog::accept();
}

void CPasswordDialog::accept() { OnOK(); }
