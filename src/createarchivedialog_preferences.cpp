// Adapted from CCompressDialog format options in CompressDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "createarchivedialog.h"
#include <QSignalBlocker>
#include <QStandardPaths>

QString CreateArchiveDialog::compressionSettingsPath() {
    const QString overridden = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
    if (!overridden.isEmpty())
        return overridden;
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(directory);
    return QDir(directory).filePath("toolbar.ini");
}

int CreateArchiveDialog::FindRegistryFormat(const QString &name) const {
    return CCompressDialog::FindRegistryFormat(m_RegistryInfo, name);
}

unsigned CreateArchiveDialog::FindRegistryFormat_Always(const QString &name) {
    const int index = FindRegistryFormat(name);
    if (index >= 0)
        return (unsigned)index;
    {
        NCompression::CFormatOptions fo;
        fo.FormatID = name;
        return m_RegistryInfo.Formats.Add(fo);
    }
}

NCompression::CFormatOptions &CreateArchiveDialog::Get_FormatOptions() {
    return m_RegistryInfo.Formats[FindRegistryFormat_Always(registryFormat_)];
}

void CreateArchiveDialog::SaveOptionsInMem() {
    // Upstream saves the format that was selected before FormatChanged.
    NCompression::CFormatOptions &fo = Get_FormatOptions();
    fo.Level = level_->currentData().toUInt();
    {
        // Qt Auto uses 0; upstream uses the all-ones sentinel.
        const UInt64 dict64 = dictionary_->currentData().toULongLong();
        UInt32 dict32;
        if (dict64 == 0)
            dict32 = (UInt32)(Int32)-1;
        else {
            dict32 = (UInt32)dict64;
            if (dict64 != dict32)
                dict32 = (UInt32)(Int32)-2;
        }
        fo.Dictionary = dict32;
    }
    fo.Order = wordSize_->currentData().toUInt();
    if (fo.Order == 0)
        fo.Order = (UInt32)(Int32)-1;
    fo.Method = method_->currentText();
    fo.NumThreads = threads_->currentData().toUInt();
    if (fo.NumThreads == 0)
        fo.NumThreads = (UInt32)(Int32)-1;
    fo.BlockLogSize = solidBlock_->currentData().toUInt();
    fo.MemUse = memUse_->currentData().toString();
    if (encryption_)
        fo.EncryptionMethod = GetEncryptionMethodSpec();
}

void CreateArchiveDialog::SetMemUseCombo() {
    const QString spec = Get_FormatOptions().MemUse;
    memUse_->blockSignals(true);
    memUse_->clear();
    int selected;
    for (const auto &item : NCompressDialog::SetMemUseCombo(spec, &selected))
        memUse_->addItem(item.User, item.Spec);
    memUse_->setCurrentIndex(selected);
    memUse_->blockSignals(false);
}

// Qt adapter for FormatChanged / SetMethod2 and the registry branches of
// SetDictionary2, SetOrder2, SetSolidBlockSize2 and SetNumThreads2.
void CreateArchiveDialog::FormatChanged() {
    const QSignalBlocker levelBlock(level_), methodBlock(method_), dictBlock(dictionary_),
        orderBlock(wordSize_), solidBlock(solidBlock_), threadsBlock(threads_);
    const NCompression::CFormatOptions fo = Get_FormatOptions();
    SetMemUseCombo();
    SetLevel2();
    method_->clear();
    method_->addItems(NCompressDialog::GetMethodNames(registryFormat_));
    const int methodIndex = method_->findText(fo.Method, Qt::MatchFixedString);
    if (methodIndex >= 0)
        method_->setCurrentIndex(methodIndex);
    MethodChanged();
}

void CreateArchiveDialog::MethodChanged() {
    const QSignalBlocker dictBlock(dictionary_), orderBlock(wordSize_), solidBlock(solidBlock_),
        threadsBlock(threads_);
    const NCompression::CFormatOptions fo = Get_FormatOptions();
    // Upstream restores method-dependent values only for the saved method.
    const bool sameMethod = method_->currentText().compare(fo.Method, Qt::CaseInsensitive) == 0;
    wordSize_->setCurrentIndex(0);
    dictionary_->setCurrentIndex(0);
    solidBlock_->setCurrentIndex(-1);
    threads_->setCurrentIndex(0);
    SetDictionary2();
    SetOrder2();
    if (sameMethod) {
        const UInt64 dict = fo.Dictionary == UInt32(-2)   ? UInt64(1) << 32
                            : fo.Dictionary == UInt32(-1) ? 0
                                                          : fo.Dictionary;
        const int index = dictionary_->findData(QVariant::fromValue(quint64(dict)));
        dictionary_->setCurrentIndex(index >= 0 ? index : 0);
        const int order = wordSize_->findData(fo.Order);
        wordSize_->setCurrentIndex(order >= 0 ? order : 0);
    }
    SetSolidBlockSize2();
    if (sameMethod) {
        const int solid = solidBlock_->findData(fo.BlockLogSize);
        solidBlock_->setCurrentIndex(solid >= 0 ? solid : 0);
    }
    SetNumThreads2();
    if (sameMethod) {
        const int threads = threads_->findData(fo.NumThreads);
        threads_->setCurrentIndex(threads >= 0 ? threads : 0);
    }
    const bool compressed = level_->currentData().toUInt() != 0;
    method_->setEnabled(compressed);
    dictionary_->setEnabled(compressed && dictionary_->count() > 1);
    wordSize_->setEnabled(compressed && wordSize_->count() > 1);
    solidBlock_->setEnabled(compressed && registryFormat_ == "7z");
    SetMemoryUsage();
}
