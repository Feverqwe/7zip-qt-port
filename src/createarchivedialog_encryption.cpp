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
    encryptionForm_->addRow("Метод:", encryption_);
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
    encryptHeaders_->setEnabled(format_->currentText() == "7z");
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
