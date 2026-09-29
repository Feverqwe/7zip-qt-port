// Adapted from 7-Zip CPP/7zip/UI/FileManager/App.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "AppState.h"
#include "Panel.h"
#include "RegistryUtils.h"
#include "qtappoperationstate.h"

const unsigned kNumPanelsMax = 2;
const int kMenuCmdID_Toolbar_Start = 1070;
const int kMenuCmdID_Plugin_Start = 1100;
enum {
    kMenuCmdID_Toolbar_Add = kMenuCmdID_Toolbar_Start,
    kMenuCmdID_Toolbar_Extract,
    kMenuCmdID_Toolbar_Test,
    kMenuCmdID_Toolbar_End
};
class IQtFileManagerMenuCommandServices;

class CApp;

class CPanelCallbackImp final : public CPanelCallback {
    CApp *_app = nullptr;
    unsigned _index = 0;

  public:
    void Init(CApp *app, unsigned index) {
        _app = app;
        _index = index;
    }
    void OnTab() override;
    void SetFocusToPath(unsigned index) override;
    void OnCopy(bool move, bool copyToSame) override;
    void OnSetSameFolder() override;
    void OnSetSubFolder() override;
    void PanelWasFocused() override;
    void DragBegin() override;
    void DragEnd() override;
    void RefreshTitle(bool always) override;
};

// Qt replaces menu/toolbar handles and panel child-control construction. Native
// creation, path binding, per-panel state and release order remain CApp/CPanel.
class IQtAppPresentation {
  public:
    virtual ~IQtAppPresentation() = default;
    virtual void Initialize(CApp &app) = 0;
    virtual bool CreatePanel(CPanel &panel, unsigned index) = 0;
    virtual CPanelCallback *PanelCallback(unsigned index) = 0;
    virtual void SetListSettings(unsigned index, const CFmSettings &settings) = 0;
    virtual void ReloadToolbars(CApp &) {}
    virtual void MoveSubWindows() {}
};

class CApp {
  public:
    CApp();
    ~CApp();
    unsigned NumPanels = 2;
    unsigned LastFocusedPanel = 0;
    bool ShowSystemMenu = false;
    bool ShowStandardToolbar = true;
    bool ShowArchiveToolbar = true;
    bool ShowButtonsLables = true;
    bool LargeButtons = false;
    CAppState AppState;
    CPanelCallbackImp m_PanelCallbackImp[kNumPanelsMax];
    CPanel Panels[kNumPanelsMax];

    HRESULT Create(QWidget *window, const UString &mainPath, const UString &arcFormat,
                   int xSizes[2], bool needOpenArc, COpenResult &openRes,
                   IQtAppPresentation &presentation, const QString &settingsPath);
    HRESULT CreateOnePanel(unsigned panelIndex, const UString &mainPath, const UString &arcFormat,
                           bool needOpenArc, COpenResult &openRes);
    void SetListSettings();
    void ReadToolbar(bool smallScreen = false);
    void SaveToolbar();
    void ReloadToolbars();
    void MoveSubWindows();
    void SaveToolbarChanges();
    void SwitchStandardToolbar();
    void SwitchArchiveToolbar();
    void SwitchButtonsLables();
    void SwitchLargeButtons();
    void SetFocusedPanel(unsigned index);
    UString PrevTitle;
    void RefreshTitle(bool always = false);
    void RefreshTitlePanel(unsigned index, bool always = false);
    // Existing Qt command consumers remain behind this routing boundary until
    // their full application command bodies are ported in P0-03/P0-08.
    void OnCopy(bool move, bool copyToSame, unsigned index);
    void OnSetSameFolder(unsigned index);
    void OnSetSubFolder(unsigned index);
    void DragBegin(unsigned index);
    void DragEnd();
    void Save();
    void ReleaseApp();
    void AttachQtCommandServices(IQtFileManagerMenuCommandServices *services) {
        _qtCommandServices = services;
    }
    bool ExecuteCommand(unsigned id);
    QtAppOperationState &QtOperations() { return _qtOperations; }
    const QtAppOperationState &QtOperations() const { return _qtOperations; }

    unsigned GetFocusedPanelIndex() const { return LastFocusedPanel; }
    bool IsPanelVisible(unsigned index) const { return NumPanels > 1 || index == LastFocusedPanel; }
    CPanel &GetFocusedPanel() { return Panels[GetFocusedPanelIndex()]; }
    void SetFocusToLastItem() { Panels[LastFocusedPanel].SetFocusToLastRememberedItem(); }

  private:
    QtAppOperationState _qtOperations;
    IQtFileManagerMenuCommandServices *_qtCommandServices = nullptr;
    QPointer<QWidget> _window;
    IQtAppPresentation *_presentation = nullptr;
    QString _settingsPath;
};
