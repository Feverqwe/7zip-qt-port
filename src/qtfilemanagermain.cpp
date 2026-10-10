#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "Common/Common.h"
#include "mainwindow.h"
#include "qtfilemanagerstartup.h"
#include "qtmemorylockservices.h"
#ifdef _WIN32
#include "../third_party/7zip/C/Alloc.h"
#endif
#include <QEventLoop>
#include <QScopeGuard>

int RunSevenzipFileManager(QApplication &application) {
    return NQtFileManager::WinMain([&] {
#if defined(_WIN32) && defined(Z7_LARGE_PAGES)
        z7_LargePage_Set(0, 0, 0);
#endif
        LoadLangOneTime();
#ifdef Q_OS_WIN
        const auto startup = WinMain2(UString(GetCommandLineW()));
#else
        const QStringList arguments = application.arguments();
        const auto startup = WinMain2(UString(arguments.value(1).toStdWString().c_str()),
                                      UString(arguments.mid(2).join(' ').toStdWString().c_str()));
#endif
        const QtMemoryLockServices memoryLock;
        NQtFileManager::SetMemoryLock(FmSettingsPath(), memoryLock);
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
