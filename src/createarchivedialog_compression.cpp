#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "createarchivedialog.h"

void CreateArchiveDialog::createCompressionOptions(QFormLayout *form) {
    auto *compression = new QGroupBox("Параметры сжатия", this);
    auto *compressionForm = new QFormLayout(compression);
    level_ = new QComboBox(compression);
    level_->setObjectName("compressionLevel");
    compressionForm->addRow("Уровень сжатия:", level_);
    method_ = new QComboBox(compression);
    method_->setObjectName("compressionMethod");
    compressionForm->addRow("Метод сжатия:", method_);
    dictionary_ = new QComboBox(compression);
    dictionary_->setObjectName("compressionDictionary");
    dictionary_->addItem("Автоматически", 0);
    compressionForm->addRow("Размер словаря:", dictionary_);
    wordSize_ = new QComboBox(compression);
    wordSize_->setObjectName("compressionWordSize");
    wordSize_->addItem("Автоматически", 0);
    compressionForm->addRow("Размер слова:", wordSize_);
    solidBlock_ = new QComboBox(compression);
    solidBlock_->setObjectName("compressionSolidBlock");
    solidBlock_->addItem("Автоматически", 0);
    compressionForm->addRow("Размер непрерывного блока:", solidBlock_);
    auto *cpuRow = new QWidget(compression);
    auto *cpuLayout = new QHBoxLayout(cpuRow);
    cpuLayout->setContentsMargins(0, 0, 0, 0);
    threads_ = new QComboBox(cpuRow);
    threads_->setObjectName("compressionThreads");
    hardwareThreads_ = new QLabel(cpuRow);
    hardwareThreads_->setObjectName("compressionHardwareThreads");
    cpuLayout->addWidget(threads_, 1);
    cpuLayout->addWidget(hardwareThreads_);
    compressionForm->addRow("Потоки CPU:", cpuRow);
    memoryUsage_ = new QLabel(compression);
    memoryUsage_->setObjectName("compressionMemoryUsage");
    auto *memoryRow = new QWidget(compression);
    auto *memoryLayout = new QHBoxLayout(memoryRow);
    memoryLayout->setContentsMargins(0, 0, 0, 0);
    memUse_ = new QComboBox(memoryRow);
    memUse_->setObjectName("compressionMemoryLimit");
    registryFormat_ = format_->currentText();
    SetMemUseCombo();
    memoryLayout->addWidget(memoryUsage_);
    memoryLayout->addWidget(memUse_, 1);
    compressionForm->addRow("Память для сжатия:", memoryRow);
    decompressionMemory_ = new QLabel(compression);
    decompressionMemory_->setObjectName("compressionDecompressionMemory");
    compressionForm->addRow("Память для распаковки:", decompressionMemory_);
    form->addRow(compression);
    connect(format_, &QComboBox::currentTextChanged, this, [this] {
        SaveOptionsInMem();
        registryFormat_ = format_->currentText();
        SetMemUseCombo();
        method_->clear();
        SetLevel2();
        method_->addItems(NCompressDialog::GetMethodNames(format_->currentText()));
        solidBlock_->setEnabled(format_->currentText() == "7z" &&
                                level_->currentData().toInt() != 0);
    });
    const auto updateMethodOptions = [this] {
        SetOrder2();
        SetDictionary2();
        SetSolidBlockSize2();
        SetNumThreads2();
        const bool compressed = level_->currentData().toInt() != 0;
        method_->setEnabled(compressed);
        dictionary_->setEnabled(compressed && dictionary_->count() > 1);
        wordSize_->setEnabled(compressed && wordSize_->count() > 1);
        solidBlock_->setEnabled(compressed && format_->currentText() == "7z");
    };
    connect(level_, &QComboBox::currentIndexChanged, this, updateMethodOptions);
    connect(method_, &QComboBox::currentTextChanged, this, updateMethodOptions);
    connect(dictionary_, &QComboBox::currentIndexChanged, this, [this] {
        SetSolidBlockSize2();
        SetNumThreads2();
    });
    connect(threads_, &QComboBox::currentIndexChanged, this, [this] { SetMemoryUsage(); });
    connect(solidBlock_, &QComboBox::currentIndexChanged, this, [this] { SetNumThreads2(); });
    connect(memUse_, &QComboBox::currentIndexChanged, this, [this] { SetNumThreads2(); });
    SetLevel2();
    method_->addItems(NCompressDialog::GetMethodNames(format_->currentText()));
}

// Adapted from CCompressDialog::SetLevel2 / SetNearestSelectComboBox (0766b73).
// Registry format defaults are replaced by the current Qt selection (initially 5).
void CreateArchiveDialog::SetLevel2() {
    const UInt32 level = level_->currentIndex() < 0 ? 5 : level_->currentData().toUInt();
    level_->clear();
    const auto &fi = NCompressDialog::GetFormatInfo(format_->currentText());
    const QString labels[] = {"Без сжатия", "Быстрое",      {}, "Быстро", {}, "Обычное",
                              {},           "Максимальное", {}, "Ультра"};
    for (unsigned i = 0; i < sizeof(UInt32) * 8; i++) {
        const UInt32 mask = fi.LevelsMask >> i;
        if (mask & 1) {
            QString text = QString::number(i);
            if (i < Z7_ARRAY_SIZE(labels) && !labels[i].isEmpty())
                text += " - " + labels[i];
            level_->addItem(text, i);
        }
    }
    for (int i = level_->count() - 1; i >= 0; i--)
        if (level_->itemData(i).toUInt() <= level) {
            level_->setCurrentIndex(i);
            return;
        }
    if (level_->count() > 0)
        level_->setCurrentIndex(0);
}

void CreateArchiveDialog::SetOrder2() {
    const int previous = wordSize_->currentData().toInt();
    const auto method =
        NCompressDialog::GetMethodID(method_->currentText(), format_->currentText());
    const auto options = NCompressDialog::SetOrder2(method, level_->currentData().toUInt());
    wordSize_->clear();
    wordSize_->addItem(options.Values.isEmpty()
                           ? QStringLiteral("Автоматически")
                           : QStringLiteral("Автоматически (%1)").arg(options.AutoOrder),
                       0);
    for (int order : options.Values)
        wordSize_->addItem(QString::number(order), order);
    const int index = wordSize_->findData(previous);
    wordSize_->setCurrentIndex(index >= 0 ? index : 0);
}

void CreateArchiveDialog::SetDictionary2() {
    const quint64 previous = dictionary_->currentData().toULongLong();
    const auto method =
        NCompressDialog::GetMethodID(method_->currentText(), format_->currentText());
    const auto options = NCompressDialog::SetDictionary2(method, level_->currentData().toUInt());
    dictionary_->clear();
    dictionary_->addItem(options.AutoDict == 0
                             ? QStringLiteral("Автоматически")
                             : QStringLiteral("Автоматически (%1)")
                                   .arg(NCompressDialog::Combo_AddDict2(options.AutoDict)),
                         QVariant::fromValue(quint64(0)));
    for (quint64 size : options.Values)
        dictionary_->addItem(NCompressDialog::Combo_AddDict2(size), QVariant::fromValue(size));
    const int index = dictionary_->findData(QVariant::fromValue(previous));
    dictionary_->setCurrentIndex(index >= 0 ? index : 0);
}

void CreateArchiveDialog::SetSolidBlockSize2() {
    const UInt32 previous =
        solidBlock_->currentIndex() < 0 ? UInt32(-1) : solidBlock_->currentData().toUInt();
    const auto method =
        NCompressDialog::GetMethodID(method_->currentText(), format_->currentText());
    const auto options = NCompressDialog::SetSolidBlockSize2(
        format_->currentText(), level_->currentData().toUInt(), method, GetDict2());
    solidBlock_->clear();
    solidBlock_->addItem(options.Values.isEmpty()
                             ? QStringLiteral("Автоматически")
                             : QStringLiteral("Автоматически (%1)")
                                   .arg(NCompressDialog::Add_Size(options.AutoSolid)),
                         UInt32(-1));
    for (UInt32 log : options.Values) {
        const QString label = log == 0    ? QStringLiteral("Без непрерывного блока")
                              : log == 64 ? QStringLiteral("Непрерывный (все файлы)")
                                          : NCompressDialog::Add_Size(UInt64(1) << log);
        solidBlock_->addItem(label, log);
    }
    const int index = solidBlock_->findData(previous);
    solidBlock_->setCurrentIndex(index >= 0 ? index : 0);
}
