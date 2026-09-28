#include "createarchivedialog.h"

UInt64 CreateArchiveDialog::GetDict2() const {
    const UInt64 dict = dictionary_->currentData().toULongLong();
    if (dict != 0)
        return dict;
    const auto method =
        NCompressDialog::GetMethodID(method_->currentText(), format_->currentText());
    const auto options = NCompressDialog::SetDictionary2(method, level_->currentData().toUInt());
    return options.AutoDict ? UInt64(options.AutoDict) : UInt64(-1);
}

void CreateArchiveDialog::SetNumThreads2() {
    const UInt32 previous = threads_->currentData().toUInt();
    const auto method =
        level_->currentData().toUInt() == 0
            ? NCompressDialog::kCopy
            : NCompressDialog::GetMethodID(method_->currentText(), format_->currentText());
    const auto options = NCompressDialog::SetNumThreads2(
        format_->currentText(), method, level_->currentData().toUInt(), GetDict2(),
        solidBlock_->currentData().toUInt(), system_);
    threads_->blockSignals(true);
    threads_->clear();
    threads_->addItem(QStringLiteral("Автоматически (%1)").arg(options.AutoNumThreads), 0u);
    for (UInt32 count : options.Values)
        threads_->addItem(QString::number(count), count);
    const int index = threads_->findData(previous);
    threads_->setCurrentIndex(index >= 0 ? index : 0);
    threads_->setEnabled(!options.Values.isEmpty());
    threads_->blockSignals(false);
    hardwareThreads_->setText(
        system_.NumCPUs == system_.NumHardwareThreads
            ? QStringLiteral("/ %1").arg(system_.NumCPUs)
            : QStringLiteral("/ %1 / %2").arg(system_.NumCPUs).arg(system_.NumHardwareThreads));
    SetMemoryUsage();
}

void CreateArchiveDialog::SetMemoryUsage() {
    const auto method =
        level_->currentData().toUInt() == 0
            ? NCompressDialog::kCopy
            : NCompressDialog::GetMethodID(method_->currentText(), format_->currentText());
    const auto options = NCompressDialog::SetNumThreads2(
        format_->currentText(), method, level_->currentData().toUInt(), GetDict2(),
        solidBlock_->currentData().toUInt(), system_);
    const UInt32 numThreads = threads_->currentData().toUInt() ? threads_->currentData().toUInt()
                                                               : options.AutoNumThreads;
    UInt64 decompressMemory;
    const UInt64 usage = NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
        format_->currentText(), method, level_->currentData().toUInt(), numThreads, GetDict2(),
        solidBlock_->currentData().toUInt(), decompressMemory);
    memoryUsage_->setText(usage == UInt64(-1) ? QStringLiteral("Неизвестно")
                                              : NCompressDialog::AddMemUsage(usage));
    decompressionMemory_->setText(decompressMemory == UInt64(-1)
                                      ? QStringLiteral("Неизвестно")
                                      : NCompressDialog::AddMemUsage(decompressMemory));
    memoryUsage_->setToolTip(system_.RamSizeDefined
                                 ? QStringLiteral("Автоматический лимит: %1 (80%)")
                                       .arg(NCompressDialog::AddMemUsage(system_.RamUsageAuto))
                                 : QStringLiteral("Размер RAM не определён"));
}
