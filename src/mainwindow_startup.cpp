#include "mainwindow.h"
#include "qtfilemanagerinitservices.h"

class QtFileManagerInitServices final : public IQtFileManagerInitServices {
    MainWindow &window_;
    const QtFileManagerStartup *startup_;

  public:
    QtFileManagerInitServices(MainWindow &window, const QtFileManagerStartup *startup)
        : window_(window), startup_(startup) {}

    bool Create(int x, int y, int width, int height, bool panelsDefined,
                UInt32 splitterPos) override {
        if (width != NQtFileManager::WindowUseDefault)
            window_.resize(width, height);
        if (x != NQtFileManager::WindowUseDefault)
            window_.move(x, y);
        window_.splitterPos_ = panelsDefined ? static_cast<int>(splitterPos) : window_.width() / 2;
        window_.createFileManagerWindow(startup_, panelsDefined);
        return true;
    }

    bool GetPlacement(QtFileManagerPlacement &placement) const override {
        const QRect rect = window_.geometry();
        placement.normalPosition = {rect.x(), rect.y(), rect.x() + rect.width(),
                                    rect.y() + rect.height()};
        placement.showCmd = NQtFileManager::NShowCommand::Normal;
        return true;
    }

    void SetPlacement(const QtFileManagerPlacement &placement) override {
        const auto &rect = placement.normalPosition;
        window_.setGeometry(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        Show(static_cast<int>(placement.showCmd));
    }

    void Show(int command) override {
        // QWidget construction remains hidden until its caller calls show().
        // Set the source show state now so the first exposure has that state.
        Qt::WindowStates state = Qt::WindowNoState;
        if (command == NQtFileManager::NShowCommand::Maximized)
            state = Qt::WindowMaximized;
        else if (command == NQtFileManager::NShowCommand::Minimized ||
                 command == NQtFileManager::NShowCommand::Minimize ||
                 command == NQtFileManager::NShowCommand::MinNoActivate ||
                 command == NQtFileManager::NShowCommand::ForceMinimize)
            state = Qt::WindowMinimized;
        window_.setWindowState(state);
    }
};

void MainWindow::initializeFileManagerWindow(const QtFileManagerStartup *startup) {
    QtFileManagerInitServices services(*this, startup);
    if (!NQtFileManager::InitInstance(NQtFileManager::NShowCommand::Normal, app_, services,
                                      toolbarSettingsPath()))
        throw E_FAIL;
}
