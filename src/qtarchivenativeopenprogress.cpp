#include "qtarchivenativeopenprogress.h"

HRESULT CNativeOpenProgress::Open_CheckBreak() {
    return _source ? _source->Open_CheckBreak()
                   : (_progress ? _progress->SetCompleted(nullptr) : S_OK);
}
HRESULT CNativeOpenProgress::Open_SetTotal(const UInt64 *files, const UInt64 *bytes) {
    return _source ? _source->Open_SetTotal(files, bytes)
                   : (_progress && bytes ? _progress->SetTotal(*bytes) : Open_CheckBreak());
}
HRESULT CNativeOpenProgress::Open_SetCompleted(const UInt64 *files, const UInt64 *bytes) {
    return _source ? _source->Open_SetCompleted(files, bytes)
                   : (_progress ? _progress->SetCompleted(bytes) : S_OK);
}
HRESULT CNativeOpenProgress::Open_Finished() { return Open_CheckBreak(); }
HRESULT CNativeOpenProgress::Open_CryptoGetTextPassword(BSTR *password) {
    HRESULT result;
    if (_source)
        result = _source->Open_CryptoGetTextPassword(password);
    else {
        CMyComPtr<ICryptoGetTextPassword> crypto;
        if (_progress)
            _progress->QueryInterface(IID_ICryptoGetTextPassword, (void **)&crypto);
        result = crypto ? crypto->CryptoGetTextPassword(password) : E_NOINTERFACE;
    }
    if (result == S_OK)
        Password = QString::fromWCharArray(*password);
    return result;
}
