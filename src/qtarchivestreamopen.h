#pragma once
#include "7zip/UI/FileManager/IFolder.h"

// A file-only manager must materialize an owned nested stream at the source
// parent-link path, so later source update/changed-file checks see the same file.
class IQtArchiveStreamOpenServices {
  public:
    virtual ~IQtArchiveStreamOpenServices() = default;
    virtual HRESULT OpenOwnedStreamFile(IInStream *stream, const wchar_t *path,
                                        const wchar_t *format, IFolderFolder **folder,
                                        IProgress *progress) = 0;
};
