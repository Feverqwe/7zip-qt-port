// SPDX-License-Identifier: MIT
#pragma once
#include "7zip/Archive/IArchive.h"
#include "asymmetrictrust.h"
#include "extverityreader.h"
#include <memory>

// Explicit properties travel through the original SDK opening path. The
// immutable trust owner is shared with streams retained after handler release.
class CExtVerityPolicy {
    std::shared_ptr<sevenzip_asymmetric_trust> trust_;
    bool require_ = false;

  public:
    HRESULT SetProperties(const wchar_t *const *, const PROPVARIANT *, UInt32);
    ext_verity_signature_options SignatureOptions() const { return {trust_.get(), require_}; }
};
