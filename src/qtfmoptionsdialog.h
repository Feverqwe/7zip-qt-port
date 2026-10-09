#pragma once
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include <QDialog>
#include <functional>
#include <memory>
struct IFolderManager;
class IQtAssociationServices;
class CSystemPage;
class CEditPage;
class CFoldersPage;
class CSettingsPage;
class CMenuPage;
class CLangPage;
class QDialogButtonBox;

// Qt property-sheet services for the native six-page OptionsDialog container.
class QtFmOptionsDialog final : public QDialog {
    Q_OBJECT
    CEditPage *_editPage;
    CFoldersPage *_foldersPage;
    CSettingsPage *_settingsPage;
    CMenuPage *_menuPage;
    CLangPage *_langPage = nullptr;
    CSystemPage *_systemPage;
    bool _applying = false;
    QDialogButtonBox *_buttons;
    void updateApplyButton();
    void changeEvent(QEvent *event) override;
    void reloadLangItems();
    void showPageHelp();
    void closeEvent(QCloseEvent *event) override;

  public:
    std::function<void(const CFmSettings &)> Applied;
    explicit QtFmOptionsDialog(const QString &settingsPath, QWidget *parent = nullptr,
                               IFolderManager *manager = nullptr,
                               std::shared_ptr<IQtAssociationServices> associations = {});
    CFmSettings settings() const;
    void apply();
    void accept() override;
    void reject() override;
};
