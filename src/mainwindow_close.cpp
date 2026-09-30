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
        for (const auto &ui : window_.panelUi_) {
            if (ui.table)
                ui.table->setAcceptDrops(false);
            if (ui.iconList)
                ui.iconList->setAcceptDrops(false);
        }
    }
    void QuiesceOperations() override { window_.quiesceOperations(); }
    bool WindowWasCreated() const override {
        return window_.app_.Panels[0].PanelCreated || window_.app_.Panels[1].PanelCreated;
    }
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
        if (window_.app_.NumPanels == 2 && window_.panelSplitter_)
            window_.splitterPos_ = window_.panelSplitter_->sizes().value(0);
        info.splitterPos = static_cast<UInt32>(qMax(1, window_.splitterPos_));
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
    for (const auto &ui : panelUi_) {
        if (ui.pathCombo)
            disconnect(ui.pathCombo, nullptr, this, nullptr);
        if (ui.path)
            disconnect(ui.path, nullptr, this, nullptr);
    }
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
