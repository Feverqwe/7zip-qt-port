// UpdateCallbackAgent.h
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.

#ifndef ZIP7_INC_UPDATE_CALLBACK_AGENT_H
#define ZIP7_INC_UPDATE_CALLBACK_AGENT_H

#include "archive/callbacks/UpdateCallback.h"

#include "archive/agent/IFolderArchive.h"

class CUpdateCallbackAgent Z7_final : public IUpdateCallbackUI {
    Z7_IFACE_IMP(IUpdateCallbackUI)

    CMyComPtr<ICryptoGetTextPassword2> _cryptoGetTextPassword;
    CMyComPtr<IFolderArchiveUpdateCallback> Callback;
    CMyComPtr<IFolderArchiveUpdateCallback2> Callback2;
    CMyComPtr<ICompressProgressInfo> _compressProgress;

  public:
    void SetCallback(IFolderArchiveUpdateCallback *callback);
};

#endif
