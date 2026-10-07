#pragma once
#include "CPP/7zip/UI/Common/OpenArchive.h"

// Per-opening integration lease. Source Agent owns opening policy and binding;
// ArchiveBackend supplies the official engine and its file-only metadata view.
class IQtAgentOpenServices : public IQtArchiveOpenService {
  public:
    virtual HRESULT LoadCodecs(const CCodecs *&codecs) = 0;
    virtual IUnknown *Resource() const = 0;
};
