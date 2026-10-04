#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "createarchivedialog.h"
#include "qtguicommand.h"
#include "qtshellarchiveservices.h"
#include <QApplication>
#include <QEventLoop>
#include <memory>

int RunSevenzipGUI(QApplication &application) {
    // The helper owns sequential source modals, without a File Manager window.
    // Closing one modal must not terminate the next source workflow stage.
    application.setQuitOnLastWindowClosed(false);
    LoadLangOneTime();
    ArchiveBackend backend;
    std::unique_ptr<QtShellArchiveServices> services;
    return SevenzipGUI([&]() -> QtShellArchiveServices & {
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
    });
}
