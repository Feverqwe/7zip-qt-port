#include "Common/Common.h"
#include "filemanager/qtfilemanagerinitservices.h"
#include "filemanager/qtfilemanagerstartup.h"
#include "filemanager/window/mainwindow.h"
#include "fileops/links/qtlinkservices.h"
#include "localization/LangUtils.h"
#include "platform/associations/qtassociationfileopen.h"
#include "platform/memory/qtmemorylockservices.h"
#ifdef Q_OS_MACOS
#include "app/qtapplicationstartup.h"
#endif
#ifdef _WIN32
#include "C/Alloc.h"
#endif
#include <QEventLoop>
#include <QFileInfo>
#include <QScopeGuard>
#include <memory>

MainWindow *OpenSevenzipAssociatedFile(const QString &path) {
    auto backend = std::make_unique<ArchiveBackend>();
    QtFileManagerStartup startup;
    startup.FullPath = QFileInfo(path).absolutePath().toStdWString().c_str();
    auto receiver = std::make_unique<MainWindow>(backend.get(), std::function<bool(const QUrl &)>{},
                                                 nullptr, &startup);
    receiver->setAttribute(Qt::WA_DeleteOnClose);
    auto *window = receiver.get();
    auto *service = backend.get();
    service->setParent(window);
    backend.release();
    // The original independent FM launch loads its codec owner before opening
    // the requested archive. A macOS file-open event must honor that same
    // prerequisite without sharing another window's operation state.
    QObject::connect(
        service, &ArchiveBackend::codecCatalogReady, window,
        [window, path](bool success, const QString &message) {
            if (success)
                window->openPath(path);
            else
                QMessageBox::critical(window, QStringLiteral("7-Zip"), message);
        },
        Qt::SingleShotConnection);
    window->show();
    service->discoverCodecCatalog();
    return receiver.release();
}

int RunSevenzipDesktop(QApplication &application) {
    QtAssociationFileOpen associatedFiles(application);
    QObject::connect(&associatedFiles, &QtAssociationFileOpen::OpenRequested, &application,
                     [](const QString &path) { OpenSevenzipAssociatedFile(path); });
#ifdef Q_OS_MACOS
    // Cocoa delivers initial documents before didFinishLaunching. Await that
    // native transaction; a single processEvents flush can return too early.
    if (!QtWaitForMacApplicationLaunch())
        return 0;
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    const bool associatedStartup = associatedFiles.Activate();
#else
    associatedFiles.Activate();
    const bool associatedStartup = false;
#endif
    return RunSevenzipFileManager(application, associatedStartup);
}

int RunSevenzipFileManager(QApplication &application, bool associatedStartup) {
    return NQtFileManager::WinMain([&] {
        NQtFileManager::GetRamInfo();
#if defined(_WIN32) && defined(Z7_LARGE_PAGES)
        z7_LargePage_Set(0, 0, 0);
#endif
        LoadLangOneTime();
        QtFileManagerStartup startup;
        if (!associatedStartup) {
#ifdef Q_OS_WIN
            startup = WinMain2(UString(GetCommandLineW()));
            // Qt's main() has no nCmdShow parameter. Recover the original WinMain
            // launch value before a progress dialog can consume the first ShowWindow.
            STARTUPINFOW nativeStartup{};
            nativeStartup.cb = sizeof(nativeStartup);
            GetStartupInfoW(&nativeStartup);
            startup.ShowCommand = nativeStartup.dwFlags & STARTF_USESHOWWINDOW
                                      ? nativeStartup.wShowWindow
                                      : NQtFileManager::NShowCommand::Default;
#else
            const QStringList arguments = application.arguments();
            startup = WinMain2(UString(arguments.value(1).toStdWString().c_str()),
                               UString(arguments.mid(2).join(' ').toStdWString().c_str()));
#endif
        }
        const QtMemoryLockServices memoryLock;
        NQtFileManager::SetMemoryLock(FmSettingsPath(), memoryLock);
        QtInitializeLinkPlatform();
        if (associatedStartup)
            return application.exec();
        ArchiveBackend backend;
        if (startup.NeedOpenArc) {
            // The source first archive open loads Global_Codecs synchronously.
            // Await the existing backend discovery before CApp::Create so its
            // initial BindToPath uses that same retained native catalog.
            QEventLoop discovery;
            bool ready = false;
            QString error;
            QObject::connect(&backend, &ArchiveBackend::codecCatalogReady, &discovery,
                             [&](bool success, const QString &message) {
                                 ready = success;
                                 error = message;
                                 discovery.quit();
                             });
            if (backend.discoverCodecCatalog())
                discovery.exec();
            if (!ready)
                throw UString(error.toStdWString().c_str());
        }
        try {
            const bool quitOnClose = application.quitOnLastWindowClosed();
            const auto restore =
                qScopeGuard([&] { application.setQuitOnLastWindowClosed(quitOnClose); });
            application.setQuitOnLastWindowClosed(false);
            MainWindow window(&backend, {}, nullptr, &startup);
            if (startup.ShowCommand != NQtFileManager::NShowCommand::Hide)
                window.show();
            application.setQuitOnLastWindowClosed(quitOnClose);
            if (!startup.NeedOpenArc)
                backend.discoverCodecCatalog();
            return application.exec();
        } catch (const QtFileManagerStartupFailure &failure) {
            // WM_CREATE refusal makes InitInstance fail and WinMain2 returns
            // FALSE. E_ABORT skips the error message in the original body.
            if (!failure.Message.IsEmpty())
                QMessageBox::critical(nullptr, QStringLiteral("7-Zip"),
                                      QString::fromWCharArray(failure.Message));
            return 0;
        }
    });
}
