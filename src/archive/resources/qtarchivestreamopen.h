#pragma once
#include "7zip/UI/FileManager/IFolder.h"

// Report whether the source parent-link path owns a materialized file, so
// changed-file checks and write-back retain their original lifetime contract.
class IQtArchiveStreamOpenServices {
  public:
    virtual ~IQtArchiveStreamOpenServices() = default;
    virtual HRESULT OpenOwnedStreamFile(IInStream *stream, const wchar_t *path,
                                        const wchar_t *format, IFolderFolder **folder,
                                        IProgress *progress, bool materialize,
                                        bool *materialized) = 0;
};
