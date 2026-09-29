#include "mainwindow.h"
#include "qtfilemanagercommands.h"
#include "qtfilemanagerwindowservices.h"
#include <QCloseEvent>

class QtFileManagerWindowServices final : public IQtFileManagerWindowServices {
    MainWindow &window_;

  public:
    explicit QtFileManagerWindowServices(MainWindow &window) : window_(window) {}
    bool CanCloseNow() const override {
        const QWidget *modal = QApplication::activeModalWidget();
        return !window_.sourceArchiveProgress_ &&
               !(modal && (modal == &window_ || window_.isAncestorOf(modal)));
    }
    void Deactivate() override {
        window_.app_.QtOperations().Close();
        if (window_.autoRefreshTimer_)
            window_.autoRefreshTimer_->stop();
        for (auto &panel : window_.app_.Panels)
            panel.Disable_Processing_Timer_Notify_StatusBar();
        window_.setEnabled(false);
    }
    void ExitExternalWorkers(bool hardExit) override {
        for (auto &panel : window_.app_.Panels)
            panel.ExitExternalWorkers(hardExit);
    }
    void RevokeDropTarget() override {
        window_.setAcceptDrops(false);
        window_.table_->setAcceptDrops(false);
        window_.iconList_->setAcceptDrops(false);
    }
    void QuiesceOperations() override { window_.quiesceOperations(); }
    bool WindowWasCreated() const override { return window_.app_.Panels[0].PanelCreated; }
    void SaveWindowInfo() override {
        CWindowInfo info{};
        bool positionDefined = false, panelsDefined = false;
        info.Read(positionDefined, panelsDefined, MainWindow::toolbarSettingsPath());
        const QRect geometry =
            window_.isMaximized() ? window_.normalGeometry() : window_.geometry();
        info.rect = {geometry.x(), geometry.y(), geometry.x() + geometry.width(),
                     geometry.y() + geometry.height()};
        info.maximized = window_.isMaximized();
        info.numPanels = window_.app_.NumPanels;
        info.currentPanel = window_.app_.LastFocusedPanel;
        // There is no visible splitter until P0-08. Preserve its native saved
        // coordinate rather than overwriting it with a single-pane invention.
        if (!panelsDefined)
            info.splitterPos = window_.width() / 2;
        info.Save(MainWindow::toolbarSettingsPath());
    }
};

void MainWindow::closeEvent(QCloseEvent *event) {
    QtFileManagerWindowServices services(*this);
    if (WndProc(QtFileManagerMessage::Close, app_, services))
        event->accept();
    else
        event->ignore();
}

void MainWindow::quiesceOperations() {
    disconnect(pathCombo_, nullptr, this, nullptr);
    disconnect(path_, nullptr, this, nullptr);
    disconnect(backend_, nullptr, this, nullptr);
    disconnect(filesystemScanner_, nullptr, this, nullptr);
    filesystemScanner_->cancelAndWait();
    backend_->cancelAndWait();
    if (fileDelete_) {
        disconnect(fileDelete_, nullptr, this, nullptr);
        fileDelete_->cancel();
        fileDelete_->wait();
    }
    if (fileTransfer_) {
        disconnect(fileTransfer_, nullptr, this, nullptr);
        fileTransfer_->cancel();
        fileTransfer_->wait();
    }
    if (volumeOperation_) {
        disconnect(volumeOperation_, nullptr, this, nullptr);
        volumeOperation_->cancel();
        volumeOperation_->wait();
    }
    closeProgressDialog();
    if (fileTransferProgress_)
        fileTransferProgress_->close();
    if (volumeProgress_)
        volumeProgress_->close();
}
