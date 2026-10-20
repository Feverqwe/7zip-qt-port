// SPDX-License-Identifier: MIT
// SDK property conversion follows 7-Zip NtfsHandler::SetProperties; the
// archive trust/configuration owner has no upstream userspace counterpart.
#include "extveritypolicy.h"
#include "7zip/Common/MethodProps.h"
#include "Common/MyString.h"
#include <cerrno>
#include <vector>

HRESULT CExtVerityPolicy::SetProperties(const wchar_t *const *names, const PROPVARIANT *values,
                                        UInt32 numProps) {
    CExtVerityPolicy next;
    std::vector<AString> encoded;
    for (UInt32 i = 0; i < numProps; ++i) {
        const wchar_t *name = names[i];
        const PROPVARIANT &prop = values[i];
        if (StringsAreEqualNoCase_Ascii(name, "verity.require")) {
            RINOK(PROPVARIANT_to_bool(prop, next.require_))
        } else if (StringsAreEqualNoCase_Ascii(name, "verity.trust")) {
            if (prop.vt != VT_BSTR)
                return E_INVALIDARG;
            UString text = prop.bstrVal;
            if (text.IsEmpty() || !text.IsAscii())
                return E_INVALIDARG;
            AString bytes;
            bytes.SetFromWStr_if_Ascii(text);
            encoded.push_back(std::move(bytes));
        } else if (IsString1PrefixedByString2_NoCase_Ascii(name, "mt")) {
        } else if (IsString1PrefixedByString2_NoCase_Ascii(name, "memuse")) {
        } else
            return E_INVALIDARG;
    }
    std::vector<sevenzip_certificate> certificates;
    certificates.reserve(encoded.size());
    for (const auto &text : encoded)
        certificates.push_back({text.Ptr(), text.Len(), SEVENZIP_CERTIFICATE_HEX});
    sevenzip_asymmetric_trust *trust = nullptr;
    const int status =
        sevenzip_asymmetric_trust_create(certificates.data(), certificates.size(), &trust);
    if (status)
        return status == -ENOMEM ? E_OUTOFMEMORY : E_INVALIDARG;
    next.trust_ =
        std::shared_ptr<sevenzip_asymmetric_trust>(trust, sevenzip_asymmetric_trust_destroy);
    *this = std::move(next);
    return S_OK;
}
