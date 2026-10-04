#include "createarchivedialog.h"
#include "qtlangcontrols.h"
#include <QSignalBlocker>

void CreateArchiveDialog::updateSfxControls() {
    if (!sfx_)
        return;
    configureArchiveName(format_->currentText());
    archiveNameState_.MethodID = level_->currentData().toUInt() == 0
                                     ? NCompressDialog::kCopy
                                     : archiveNameState_.GetMethodID();
    const bool wasSfx = archiveNameState_.Info.SFXMode;
    const bool sourceSupported = archiveNameState_.CheckSFXControlsEnable();
    const bool available = sourceSupported && sfxModule_.available();
    if (!available) {
        archiveNameState_.Info.SFXMode = false;
        const QSignalBlocker blocker(sfx_);
        sfx_->setChecked(false);
        if (wasSfx) {
            archiveNameState_.m_ArchivePath = name_->text();
            archiveNameState_.OnButtonSFX();
            name_->setText(archiveNameState_.m_ArchivePath);
        }
    }
    sfx_->setEnabled(available);
    if (!sfxModule_.available())
        QtBindLocalToolTip(sfx_, "An official SFX module for this platform is unavailable.",
                           "Официальный модуль SFX для этой платформы недоступен.");
    else if (!sourceSupported)
        QtBindLocalToolTip(sfx_, "The selected format or method does not support SFX.",
                           "Выбранный формат или метод не поддерживает SFX.");
    else
        QtBindLocalToolTip(sfx_, QString(), QString());
}

void CreateArchiveDialog::toggleSfx(bool enabled) {
    archiveNameState_.Info.SFXMode = enabled;
    archiveNameState_.m_ArchivePath = name_->text();
    archiveNameState_.OnButtonSFX();
    name_->setText(archiveNameState_.m_ArchivePath);
    SaveOptionsInMem();
    FormatChanged();
}
