#pragma once
#include "archivebackend.h"

struct NativeArchiveListing {
    QList<ArchiveEntry> Entries;
    ArchiveInfo Info;
    bool CanUpdate = false;
};

HRESULT ReadNativeArchiveListing(const ArchiveNativeOpen &reader, NativeArchiveListing &listing,
                                 IProgress *progress);
