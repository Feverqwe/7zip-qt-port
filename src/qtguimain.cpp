#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "createarchivedialog.h"
#include "qtguicommand.h"
#include "qtoperationendpoint.h"
#include "qtshellarchiveservices.h"
#include <QApplication>
#include <QEventLoop>
#include <memory>

int RunSevenzipGUICommand(QApplication &application, const QtGUICommandSource &command) {
    // The helper owns sequential source modals, without a File Manager window.
    // Closing one modal must not terminate the next source workflow stage.
    application.setQuitOnLastWindowClosed(false);
    LoadLangOneTime();
    ArchiveBackend backend;
    std::unique_ptr<QtShellArchiveServices> services;
    const QtGUIServiceFactory factory = [&]() -> QtShellArchiveServices & {
        // Main2 invokes the factory after Parse2 acknowledged its mapping.
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
        services = std::make_unique<QtShellArchiveServices>(
            backend, nullptr, QtShellArchiveServices::OpenSink{},
            QtShellArchiveServices::CompressSink{}, QtShellArchiveServices::ErrorSink{},
            [&backend](CCompressDialog &source, const CObjectVector<NWildcard::CCensorPath> &paths,
                       QWidget *parent) {
                return CreateArchiveDialog::PresentSource(source, paths, backend.codecCatalog(),
                                                          parent);
            },
            FmSettingsPath());
        return *services;
    };
    return SevenzipGUI(factory, command);
}

int RunSevenzipGUI(QApplication &application) {
    application.setQuitOnLastWindowClosed(false);
    const auto arguments = application.arguments();
    if (arguments.size() == 3 && arguments[1] == "--operation-channel")
        return RunGUIOperationEndpoint(application, arguments[2],
                                       [&](const QtGUICommandSource &command) {
                                           return RunSevenzipGUICommand(application, command);
                                       });
    return RunSevenzipGUICommand(application, {});
}
