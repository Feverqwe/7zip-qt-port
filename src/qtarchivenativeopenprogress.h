#pragma once
#include "7zip/UI/Common/ArchiveOpenCallback.h"
#include <QString>

// Presentation projection only; native detection/volume callbacks stay upstream.
class CNativeOpenProgress final : public IOpenCallbackUI {
    IProgress *_progress;
    IOpenCallbackUI *_source;

  public:
    QString Password;
    CNativeOpenProgress(IProgress *progress, IOpenCallbackUI *source)
        : _progress(progress), _source(source) {}
    HRESULT Open_CheckBreak() override;
    HRESULT Open_SetTotal(const UInt64 *files, const UInt64 *bytes) override;
    HRESULT Open_SetCompleted(const UInt64 *files, const UInt64 *bytes) override;
    HRESULT Open_Finished() override;
    HRESULT Open_CryptoGetTextPassword(BSTR *password) override;
};
