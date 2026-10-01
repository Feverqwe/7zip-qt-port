#pragma once
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include <QDialog>
#include <functional>
class CEditPage;
class CFoldersPage;
class CSettingsPage;
class CMenuPage;
class CLangPage;
class QDialogButtonBox;

// Qt property-sheet services for the partial native OptionsDialog container.
class QtFmOptionsDialog final : public QDialog {
    Q_OBJECT
    CEditPage *_editPage;
    CFoldersPage *_foldersPage;
    CSettingsPage *_settingsPage;
    CMenuPage *_menuPage;
    CLangPage *_langPage = nullptr;
    QDialogButtonBox *_buttons;
    void updateApplyButton();
    void changeEvent(QEvent *event) override;
    void reloadLangItems();

  public:
    std::function<void(const CFmSettings &)> Applied;
    explicit QtFmOptionsDialog(const QString &settingsPath, QWidget *parent = nullptr);
    CFmSettings settings() const;
    void apply();
    void accept() override;
};
