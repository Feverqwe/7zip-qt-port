#pragma once
#include "platform/temp/qtownedtempregistry.h"
#include <QDir>

// Forced-child-exit fixtures can leave native deployment images. Delete only
// inactive objects owned by this CTest suite, after OS-identity revalidation.
inline bool CleanupOperationTestTemps() {
    const QString root = qEnvironmentVariable("SEVENZIP_OWNED_TEMP_REGISTRY");
    if (root.isEmpty())
        return true;
    QtOwnedTempRegistry registry(root);
    for (const auto &entry : registry.Entries()) {
        if (entry.active || entry.externallyProtected)
            continue;
        auto lease = registry.ClaimForDeletion(entry);
        if (!lease || !QDir(lease->path()).removeRecursively())
            return false;
    }
    return true;
}
