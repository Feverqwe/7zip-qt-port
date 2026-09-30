#pragma once
#include "CPP/7zip/UI/FileManager/App.h"

class MainWindow;
class QMenu;

// Qt child controls and legacy settings migration for the native app owner.
class QtAppPresentation final : public IQtAppPresentation {
  public:
    explicit QtAppPresentation(MainWindow &window) : window_(window) {}
    void Initialize(CApp &app) override;
    bool CreatePanel(CPanel &panel, unsigned index) override;
    CPanelCallback *PanelCallback(unsigned index) override;
    void SetListSettings(unsigned index, const CFmSettings &settings) override;
    void ShowPanel(unsigned index, bool visible) override;
    void ReloadToolbars(CApp &app) override;
    void MoveSubWindows() override;

  private:
    MainWindow &window_;
    QMenu *editMenu_ = nullptr;
};
