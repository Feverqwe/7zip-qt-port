#include "native_mapi_loader.h"
#include "qtguicommand.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <windows.h>

int main(int argc, char **argv) {
    QApplication application(argc, argv);
    const QString provider =
        QFileInfo(qEnvironmentVariable("SEVENZIP_MAPI_TEST_PROVIDER")).canonicalFilePath();
    // Preload the exact private provider before original DllSecur startup.
    // Failure is terminal: the fixture must never call the system mail client.
    HMODULE module = LoadPrivateMAPIProvider(provider);
    if (!module)
        return 101;
    const QString before = QDir::currentPath();
    const int result = RunSevenzipGUI(application);
    QFile report(qEnvironmentVariable("SEVENZIP_MAPI_TEST_REPORT") + ".host.json");
    if (!report.open(QIODevice::WriteOnly))
        return 105;
    report.write(QJsonDocument(QJsonObject{{"cwdBefore", before},
                                           {"cwdAfter", QDir::currentPath()},
                                           {"result", result},
                                           {"provider", provider}})
                     .toJson());
    FreeLibrary(module);
    return result;
}
