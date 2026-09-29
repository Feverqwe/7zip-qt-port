// Shared native archive property view over the folder's engine metadata owner.
#pragma once
#include "7zip/Archive/IArchive.h"
#include "7zip/UI/FileManager/IFolder.h"

HRESULT CreateArchivePropertyTransport(IFolderFolder *properties, IInArchive **archive,
                                       IUnknown *engineResource = nullptr);

// Portable transport counterpart of a codec FormatInfo.UpdateEnabled lookup.
// The metadata owner exposes IOutArchive only when that engine service exists.
bool ArchiveUpdateEnabled(IInArchive *archive);
