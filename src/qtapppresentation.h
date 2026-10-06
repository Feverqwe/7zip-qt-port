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
    void RefreshFolder(unsigned index) override;
    void PrepareFolderChange(unsigned index) override;
    void ExecuteCopy(bool move, bool copyToSame, unsigned sourceIndex, const QString &destination,
                     const CRecordVector<UInt32> &indices, bool useDestPanel,
                     bool useTemp) override;
    void SetListSettings(unsigned index, const CFmSettings &settings) override;
    void ShowPanel(unsigned index, bool visible) override;
    void ReloadToolbars(CApp &app) override;
    void MoveSubWindows() override;
    void CopyFinished();
    void CompleteCopyIfReady();

  private:
    MainWindow &window_;
    QMenu *editMenu_ = nullptr;
    unsigned copySource_ = 0;
    bool copyPending_ = false;
    bool copyExecuting_ = false;
    bool copyFinished_ = false;
};
