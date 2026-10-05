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
    return CCompressDialog::FindRegistryFormat_Always(m_RegistryInfo, name);
}

NCompression::CFormatOptions &CreateArchiveDialog::Get_FormatOptions() {
    return m_RegistryInfo.Formats[FindRegistryFormat_Always(registryFormat_)];
}

void CreateArchiveDialog::SaveOptionsInMem() {
    // Upstream saves the format that was selected before FormatChanged.
    NCompressDialog::CInfo info;
    if (parameters_)
        info.Options = parameters_->text().toStdWString().c_str();
    info.Level = level_->currentData().toUInt();
    // Qt Auto uses 0; source GetComboValue(control, 1) returns all ones.
    const UInt64 dictionary = dictionary_->currentData().toULongLong();
    info.Dict64 = dictionary ? dictionary : UInt64(-1);
    const UInt32 order = wordSize_->currentData().toUInt();
    info.Order = order ? order : UInt32(-1);
    info.Method = archiveNameState_.GetMethodSpec();
    const UInt32 threads = threads_->currentData().toUInt();
    info.NumThreads = threads ? threads : UInt32(-1);
    info.EncryptionMethod =
        encryption_ ? GetEncryptionMethodSpec() : Get_FormatOptions().EncryptionMethod;
    CCompressDialog::SaveOptionsInMem(m_RegistryInfo, registryFormat_, info,
                                      solidBlock_->currentData().toUInt(),
                                      memUse_->currentData().toString(), parameters_ != nullptr);
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
    if (parameters_)
        parameters_->setText(QString::fromWCharArray(fo.Options.Ptr()));
    updateAdvancedSummary();
    SetMemUseCombo();
    SetLevel2();
    configureArchiveName(registryFormat_);
    archiveNameState_.SetMethod2(*method_, level_->currentData().toUInt(), m_RegistryInfo,
                                 [this] { MethodChanged(); });
}

QString CreateArchiveDialog::methodName() const {
    UString estimatedName;
    archiveNameState_.GetMethodSpec(estimatedName);
    return QString::fromWCharArray(estimatedName.Ptr());
}

void CreateArchiveDialog::MethodChanged() {
    const QSignalBlocker dictBlock(dictionary_), orderBlock(wordSize_), solidBlock(solidBlock_),
        threadsBlock(threads_);
    const NCompression::CFormatOptions fo = Get_FormatOptions();
    // Upstream restores method-dependent values only for the saved method.
    const bool sameMethod =
        archiveNameState_.IsMethodEqualTo(UString(fo.Method.toStdWString().c_str()));
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
    // Source SetMethod2 retains TAR's GNU/POSIX choice at its only level (0).
    method_->setEnabled(method_->count() > 1);
    dictionary_->setEnabled(compressed && dictionary_->count() > 1);
    wordSize_->setEnabled(compressed && wordSize_->count() > 1);
    solidBlock_->setEnabled(compressed && NCompressDialog::GetFormatInfo(registryFormat_).Solid_());
    SetMemoryUsage();
    updateSfxControls();
}
