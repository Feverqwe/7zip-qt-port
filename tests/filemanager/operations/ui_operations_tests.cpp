#include "window_operations.h"

void MainWindowOperationsTests::init() {
    const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
    if (!settings.isEmpty())
        QFile::remove(settings);
    SaveRegLang(UString("-"), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
}

QTEST_MAIN(MainWindowOperationsTests)
