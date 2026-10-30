#pragma once
#include "archive/hashing/HashCalc.h"
#include "archive/operations/Extract.h"
#include "filemanager/ExtractCallback.h"
#include "platform/shell/qtcontextmenuservices.h"
struct CUpdateOptions;
class CUpdateCallbackGUI;

// Engine/GUI boundaries for the original in-process CompressCall2 workflows.
// The caller retains its catalog and native opener for the synchronous call.
class IQtCompressCallServices : public virtual IQtContextMenuServices {
  public:
    virtual CCodecs &Codecs() = 0;
    virtual HRESULT LoadHashProviders(CExternalCodecs &providers) = 0;
    virtual HRESULT CompressSelection(const UString &folder, const UString &name,
                                      const UString &type, bool addExtension,
                                      const UStringVector &paths, bool email, bool showDialog,
                                      bool waitFinish);
    virtual HRESULT ExtractGUI(CCodecs &codecs, const CObjectVector<COpenType> &types,
                               UStringVector &paths, UStringVector &fullPaths,
                               const NWildcard::CCensorNode &censor, CExtractOptions &options,
                               bool showDialog, bool &messageWasDisplayed,
                               CExtractCallbackImp &callback) = 0;
    virtual HRESULT HashCalcGUI(const CExternalCodecs *providers, const NWildcard::CCensor &censor,
                                const CHashOptions &options, bool &messageWasDisplayed) = 0;
    virtual HRESULT UpdateGUI(CCodecs &, const CObjectVector<COpenType> &, const UString &,
                              NWildcard::CCensor &, CUpdateOptions &, bool, bool &,
                              CUpdateCallbackGUI &) {
        return E_NOTIMPL;
    }
    virtual void ErrorMessageHRESULT(HRESULT result) = 0;
};
