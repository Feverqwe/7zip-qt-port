#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "createarchivedialog.h"
#include "qtlangcontrols.h"

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
        solidBlock_->currentData().toUInt(), system_, Get_MemUse_Spec());
    threads_->blockSignals(true);
    threads_->clear();
    threads_->addItem(NCompressDialog::Modify_Auto(QString::number(options.AutoNumThreads)), 0u);
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
        solidBlock_->currentData().toUInt(), system_, Get_MemUse_Spec());
    const UInt32 numThreads = threads_->currentData().toUInt() ? threads_->currentData().toUInt()
                                                               : options.AutoNumThreads;
    UInt64 decompressMemory;
    const UInt64 usage = NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
        format_->currentText(), method, level_->currentData().toUInt(), numThreads, GetDict2(),
        solidBlock_->currentData().toUInt(), decompressMemory);
    memoryUsage_->setText(usage == UInt64(-1) ? QStringLiteral("?")
                                              : NCompressDialog::AddMemUsage(usage));
    decompressionMemory_->setText(decompressMemory == UInt64(-1)
                                      ? QStringLiteral("?")
                                      : NCompressDialog::AddMemUsage(decompressMemory));
    const auto mu = Get_MemUse_Spec();
    memoryUsage_->setToolTip(
        mu.IsDefined || system_.RamSizeDefined
            ? QtLocalText("Limit: %1 / RAM: %2", "Лимит: %1 / RAM: %2")
                  .arg(NCompressDialog::AddMemUsage(NCompressDialog::Get_MemUse_Bytes(mu, system_)),
                       system_.RamSizeDefined ? NCompressDialog::AddMemUsage(system_.RamSize)
                                              : QtLocalText("unknown", "не определена"))
            : QtLocalText("RAM size is unknown", "Размер RAM не определён"));
}

NCompression::CMemUse CreateArchiveDialog::Get_MemUse_Spec() const {
    NCompression::CMemUse mu;
    mu.Parse(memUse_->currentData().toString());
    return mu;
}

bool CreateArchiveDialog::checkMemoryUsage() {
    const auto options = compressionOptions();
    const auto method = options.level == 0
                            ? NCompressDialog::kCopy
                            : NCompressDialog::GetMethodID(options.method, format_->currentText());
    const auto threads = NCompressDialog::SetNumThreads2(
        format_->currentText(), method, UInt32(options.level), GetDict2(),
        solidBlock_->currentData().toUInt(), system_, options.MemUsage);
    UInt64 decompressMem;
    const UInt64 memUsage = NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
        format_->currentText(), method, UInt32(options.level),
        options.threads ? UInt32(options.threads) : threads.AutoNumThreads, GetDict2(),
        solidBlock_->currentData().toUInt(), decompressMem);
    if (memUsage != UInt64(-1) && (options.MemUsage.IsDefined || system_.RamSizeDefined)) {
        const UInt64 limit = NCompressDialog::Get_MemUse_Bytes(options.MemUsage, system_);
        if (memUsage > limit) {
            UString usage;
            LangString_OnlyFromLangFile(IDS_MEM_REQUIRED_MEM_SIZE, usage);
            if (usage.IsEmpty()) {
                usage = LangString(IDT_COMPRESS_MEMORY);
                usage.RemoveChar(L':');
            }
            UString message;
            SetErrorMessage_MemUsage(message, memUsage, system_.RamSize, limit, usage);
            QMessageBox::warning(this, windowTitle(), QString::fromWCharArray(message.Ptr()));
            return false;
        }
    }
    return true;
}
