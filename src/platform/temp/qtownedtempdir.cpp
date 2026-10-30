#include "platform/temp/qtownedtempdir.h"
#include <QDir>

QtOwnedTempDir::QtOwnedTempDir(const QString &directoryTemplate)
    : directory_(directoryTemplate.isEmpty() ? QDir::tempPath() + "/sevenzip-desktop-XXXXXX"
                                             : directoryTemplate) {
    if (directory_.isValid()) {
        lease_ = QtOwnedTempRegistry().RegisterCreated(directory_.path());
        if (!lease_)
            directory_.remove();
    }
}
