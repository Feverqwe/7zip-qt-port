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
    const int keepMethodId = archiveNameState_.GetMethodID();
    archiveNameState_.Info.SFXMode = enabled;
    // Original OnButtonClicked(IDX_COMPRESS_SFX): SetMethod(GetMethodID())
    // retains dependent controls when that method survives the SFX filter.
    const QSignalBlocker methodBlock(method_);
    archiveNameState_.SetMethod2(
        *method_, level_->currentData().toUInt(), m_RegistryInfo, [this] { MethodChanged(); },
        keepMethodId);
    method_->setEnabled(method_->count() > 1);
    archiveNameState_.m_ArchivePath = name_->text();
    archiveNameState_.OnButtonSFX();
    name_->setText(archiveNameState_.m_ArchivePath);
    SetMemoryUsage();
}
