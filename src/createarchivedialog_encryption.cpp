// Qt presentation for CompressDialog.cpp password/encryption controls (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "createarchivedialog.h"

void CreateArchiveDialog::createEncryptionOptions(QFormLayout *form) {
    auto *group = new QGroupBox("Шифрование", this);
    group->setObjectName("archiveEncryptionGroup");
    encryptionForm_ = new QFormLayout(group);
    encryptionForm_->setRowWrapPolicy(QFormLayout::WrapAllRows);
    password_ = new QLineEdit(group);
    password_->setObjectName("archivePassword");
    encryptionForm_->addRow("Пароль:", password_);
    passwordConfirmation_ = new QLineEdit(group);
    passwordConfirmation_->setObjectName("archivePasswordConfirmation");
    encryptionForm_->addRow("Повторите пароль:", passwordConfirmation_);
    showPassword_ = new QCheckBox("Показать пароль", group);
    showPassword_->setObjectName("archiveShowPassword");
    showPassword_->setChecked(m_RegistryInfo.ShowPassword);
    encryptionForm_->addRow(showPassword_);
    encryption_ = new QComboBox(group);
    encryption_->setObjectName("archiveEncryption");
    // CompressDialog.rc keeps IDT_COMPRESS_ENCRYPTION_METHOD beside its
    // combo; password labels remain above their full-width edits.
    auto *methodRow = new QWidget(group);
    auto *methodLayout = new QHBoxLayout(methodRow);
    methodLayout->setContentsMargins(0, 0, 0, 0);
    auto *methodLabel = new QLabel("Метод:", methodRow);
    methodLabel->setObjectName("archiveEncryptionMethodLabel");
    methodLabel->setBuddy(encryption_);
    methodLayout->addWidget(methodLabel);
    methodLayout->addWidget(encryption_, 1);
    encryptionForm_->addRow(methodRow);
    encryptHeaders_ = new QCheckBox("Шифровать имена файлов", group);
    encryptHeaders_->setObjectName("archiveEncryptHeaders");
    encryptHeaders_->setChecked(m_RegistryInfo.EncryptHeaders);
    encryptionForm_->addRow(encryptHeaders_);
    form->addRow(group);
    connect(showPassword_, &QCheckBox::toggled, this, &CreateArchiveDialog::UpdatePasswordControl);
    connect(format_, &QComboBox::currentTextChanged, this,
            &CreateArchiveDialog::SetEncryptionMethod);
    SetEncryptionMethod();
    UpdatePasswordControl();
}

void CreateArchiveDialog::SetEncryptionMethod() {
    CCompressDialog dialog;
    dialog.Format = format_->currentText();
    dialog.SetEncryptionMethod(m_RegistryInfo);
    encryption_->clear();
    encryption_->addItems(dialog._encryptionMethod);
    encryption_->setCurrentIndex(dialog._encryptionMethod_Index);
    const auto &fi = NCompressDialog::GetFormatInfo(format_->currentText());
    password_->parentWidget()->setEnabled(fi.Encrypt_());
    encryptHeaders_->setEnabled(fi.EncryptFileNames_());
    encryptHeaders_->setVisible(fi.EncryptFileNames_());
}

QString CreateArchiveDialog::GetEncryptionMethodSpec() const {
    CCompressDialog dialog;
    dialog.Format = registryFormat_;
    dialog.SetEncryptionMethod(m_RegistryInfo);
    dialog._encryptionMethod_Index = encryption_->currentIndex();
    return dialog.GetEncryptionMethodSpec();
}

void CreateArchiveDialog::UpdatePasswordControl() {
    const bool showPassword = showPassword_->isChecked();
    const auto mode = showPassword ? QLineEdit::Normal : QLineEdit::Password;
    password_->setEchoMode(mode);
    passwordConfirmation_->setEchoMode(mode);
    encryptionForm_->setRowVisible(passwordConfirmation_, !showPassword);
}
