// Qt/native input adapters for source-owned read-only folder locations.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyString.h"
#include "archive/backend/archivebackend.h"

HRESULT CreateFilesystemFolder(const QString &path, IFolderFolder **resultFolder);
class CProxyArc;
int QtArchiveDirectoryIndex(const CProxyArc &proxy, const QString &path);
QString QtFilesystemPath(const FString &path);
HRESULT CreateArchiveMetadata(const QList<ArchiveEntry> &entries, IFolderFolder **metadata);
HRESULT CreateArchiveFolder(const QList<ArchiveEntry> &entries, const UString &archiveType,
                            IFolderFolder **resultFolder, IUnknown *engineResource = nullptr);
