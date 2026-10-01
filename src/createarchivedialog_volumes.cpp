#include "CPP/7zip/UI/FileManager/SplitUtils.h"
#include "createarchivedialog.h"

void CreateArchiveDialog::createVolumeOptions(QFormLayout *form) {
    volume_ = new QComboBox(this);
    volume_->setObjectName("archiveVolumeSize");
    volume_->setEditable(true);
    volume_->setInsertPolicy(QComboBox::NoInsert);
    AddVolumeItems(*volume_);
    volume_->setCurrentIndex(-1);
    volume_->setEnabled(existingTarget_.isEmpty());
    volume_->setToolTip("Например: 10M или 10M 20M. Пустое поле — один архив.");
    form->addRow("Разбить на тома, байт:", volume_);
}

bool CreateArchiveDialog::checkVolumeSizes() {
    if (!volume_->isEnabled())
        return true;
    NCompressDialog::CInfo info;
    const auto result = CCompressDialog::OnOK(volume_->currentText(), info);
    if (result == CCompressDialog::kIncorrectVolumeSize) {
        QMessageBox::warning(this, "7-Zip", "Неверный размер тома.");
        volume_->setFocus();
        return false;
    }
    if (result == CCompressDialog::kConfirmSmallVolume)
        return QMessageBox::question(
                   this, "7-Zip",
                   QStringLiteral("Указан размер тома: %1 байт.\nРазбить архив на такие тома?")
                       .arg(info.VolumeSizes.Back()),
                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel) == QMessageBox::Yes;
    return true;
}
