// Qt/7zz input adapters for source-owned read-only folder locations.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyString.h"
#include "archivebackend.h"

HRESULT CreateFilesystemFolder(const QString &path, IFolderFolder **resultFolder);
HRESULT CreateArchiveFolder(const QList<ArchiveEntry> &entries, const UString &archiveType,
                            IFolderFolder **resultFolder);
