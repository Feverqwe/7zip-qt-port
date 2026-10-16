// Adapted from 7-Zip CPP/7zip/UI/FileManager/PasswordDialog.cpp/.rc (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt controls replace Win32 attachment/messages; functional handler flow stays.
#include "PasswordDialog.h"
#include "7zip/UI/FileManager/PasswordDialogRes.h"
#include "qtdialogunits.h"
#include "qtlangbuttonbox.h"
#include "qtlangcontrols.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    QtBindResourceText(this, IDD_PASSWORD, "Enter password");
    const QtDialogUnits units(font());
    const QSize margin = units.pixels(QSize(8, 8));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(margin.width(), margin.height(), margin.width(), margin.height());
    layout->setSpacing(0);
    auto *label = new QLabel(this);
    QtBindResourceText(label, IDT_PASSWORD_ENTER, "&Enter password:");
    label->setObjectName("passwordPrompt");
    label->setMinimumHeight(units.pixels(QSize(0, 8)).height());
    layout->addWidget(label);
    layout->addSpacing(units.pixels(QSize(0, 4)).height());
    _passwordEdit = new QLineEdit(this);
    _passwordEdit->setObjectName("passwordEdit");
    QtBindLangText(_passwordEdit, IDT_PASSWORD_ENTER, "Enter password:", true);
    label->setBuddy(_passwordEdit);
    _passwordEdit->setMinimumHeight(units.pixels(QSize(0, 14)).height());
    layout->addWidget(_passwordEdit);
    layout->addSpacing(units.pixels(QSize(0, 8)).height());
    _showPassword = new QCheckBox(this);
    QtBindResourceText(_showPassword, IDX_PASSWORD_SHOW, "&Show password");
    _showPassword->setObjectName("passwordShow");
    _showPassword->setMinimumHeight(units.pixels(QSize(0, 10)).height());
    layout->addWidget(_showPassword);
    _showPassword->setChecked(ShowPassword);
    SetTextSpec();
    layout->addSpacing(units.pixels(QSize(0, 12)).height());
    layout->addStretch(1);
    auto *buttons = new QtLangButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("passwordButtons");
    buttons->layout()->setSpacing(margin.width());
    for (auto *button : buttons->buttons()) {
        button->setFixedWidth(qMax(units.pixels(QSize(64, 0)).width(), button->sizeHint().width()));
        button->setMinimumHeight(units.pixels(QSize(0, 16)).height());
    }
    layout->addWidget(buttons);
    connect(_showPassword, &QCheckBox::clicked, this, &CPasswordDialog::OnButtonClicked);
    connect(buttons, &QDialogButtonBox::accepted, this, &CPasswordDialog::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    resize(units.pixels(QSize(216, 88)).expandedTo(minimumSizeHint()));
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
