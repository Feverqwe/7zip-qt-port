#pragma once
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include <QDialog>
#include <functional>
class CEditPage;
class CFoldersPage;
class CSettingsPage;
class QDialogButtonBox;

// Qt property-sheet services for the partial native OptionsDialog container.
class QtFmOptionsDialog final : public QDialog {
    Q_OBJECT
    CEditPage *_editPage;
    CFoldersPage *_foldersPage;
    CSettingsPage *_settingsPage;
    QDialogButtonBox *_buttons;
    void updateApplyButton();

  public:
    std::function<void(const CFmSettings &)> Applied;
    explicit QtFmOptionsDialog(const QString &settingsPath, QWidget *parent = nullptr);
    CFmSettings settings() const;
    void apply();
    void accept() override;
};
