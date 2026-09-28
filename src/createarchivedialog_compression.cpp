#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "createarchivedialog.h"

namespace {
void SetSolidBlockSize2(QComboBox &combo, QStringView format, UInt32 level) {
    const int previous = combo.currentData().toInt();
    combo.clear();
    combo.addItem(QStringLiteral("Автоматически"), 0);
    for (int size : NCompressDialog::SetSolidBlockSize2(format, level))
        combo.addItem(size < 1024 ? QStringLiteral("%1 МиБ").arg(size)
                                  : QStringLiteral("%1 ГиБ").arg(size / 1024),
                      size);
    const int index = combo.findData(previous);
    combo.setCurrentIndex(index >= 0 ? index : 0);
}
} // namespace

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
    solid_ = new QCheckBox("Непрерывный архив", compression);
    solid_->setObjectName("solidArchiveOption");
    solid_->setChecked(true);
    compressionForm->addRow(solid_);
    solidBlock_ = new QComboBox(compression);
    solidBlock_->setObjectName("compressionSolidBlock");
    solidBlock_->addItem("Автоматически", 0);
    compressionForm->addRow("Размер непрерывного блока:", solidBlock_);
    threads_ = new QSpinBox(compression);
    threads_->setObjectName("compressionThreads");
    threads_->setRange(0, 256);
    threads_->setSpecialValueText("Автоматически");
    compressionForm->addRow("Потоки CPU:", threads_);
    form->addRow(compression);
    connect(format_, &QComboBox::currentTextChanged, this, [this] {
        method_->clear();
        SetLevel2();
        method_->addItems(NCompressDialog::GetMethodNames(format_->currentText()));
        solid_->setEnabled(format_->currentText() == "7z");
        solidBlock_->setEnabled(format_->currentText() == "7z" && solid_->isChecked() &&
                                level_->currentData().toInt() != 0);
    });
    const auto updateMethodOptions = [this] {
        SetOrder2();
        SetDictionary2();
        SetSolidBlockSize2(*solidBlock_, format_->currentText(), level_->currentData().toUInt());
        const bool compressed = level_->currentData().toInt() != 0;
        method_->setEnabled(compressed);
        dictionary_->setEnabled(compressed && dictionary_->count() > 1);
        wordSize_->setEnabled(compressed && wordSize_->count() > 1);
        solidBlock_->setEnabled(compressed && format_->currentText() == "7z" &&
                                solid_->isChecked());
    };
    connect(level_, &QComboBox::currentIndexChanged, this, updateMethodOptions);
    connect(method_, &QComboBox::currentTextChanged, this, updateMethodOptions);
    connect(solid_, &QCheckBox::toggled, this, [this](bool enabled) {
        solidBlock_->setEnabled(format_->currentText() == "7z" && enabled &&
                                level_->currentData().toInt() != 0);
    });
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
