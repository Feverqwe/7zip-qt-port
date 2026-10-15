#pragma once
#include "7zip/Archive/IArchive.h"
#include <QList>
#include <QStringList>

struct SourceRenamedItem {
    QString OldPath;
    QString NewPath;
    bool Directory = false;
};

HRESULT ReadSourceRenamePaths(IArchiveUpdateCallback *callback,
                              const QList<SourceRenamedItem> &renamed, QStringList &paths);
