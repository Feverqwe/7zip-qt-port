// Adapted from CCompressDialog format options in CompressDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "createarchivedialog.h"
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
    FOR_VECTOR(i, m_RegistryInfo.Formats) {
        const NCompression::CFormatOptions &fo = m_RegistryInfo.Formats[i];
        if (name.compare(fo.FormatID, Qt::CaseInsensitive) == 0)
            return (int)i;
    }
    return -1;
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
    fo.MemUse = memUse_->currentData().toString();
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
