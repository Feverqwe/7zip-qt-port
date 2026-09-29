// Adapted from 7-Zip CPP/7zip/UI/FileManager/ExtractCallback.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Complete error flag resource array and GetOpenArcErrorMessage body only.
#include "ExtractCallback.h"
#include "7zip/PropID.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "Common/IntToString.h"
#include "LangUtils.h"

static const UInt32 k_ErrorFlagsIds[] = {
    IDS_EXTRACT_MSG_IS_NOT_ARC,       IDS_EXTRACT_MSG_HEADERS_ERROR,
    IDS_EXTRACT_MSG_HEADERS_ERROR,    IDS_OPEN_MSG_UNAVAILABLE_START,
    IDS_OPEN_MSG_UNCONFIRMED_START,   IDS_EXTRACT_MSG_UEXPECTED_END,
    IDS_EXTRACT_MSG_DATA_AFTER_END,   IDS_EXTRACT_MSG_UNSUPPORTED_METHOD,
    IDS_OPEN_MSG_UNSUPPORTED_FEATURE, IDS_EXTRACT_MSG_DATA_ERROR,
    IDS_EXTRACT_MSG_CRC_ERROR};

UString GetOpenArcErrorMessage(UInt32 errorFlags) {
    UString s;

    for (unsigned i = 0; i < Z7_ARRAY_SIZE(k_ErrorFlagsIds); i++) {
        const UInt32 f = (UInt32)1 << i;
        if ((errorFlags & f) == 0)
            continue;
        const UInt32 id = k_ErrorFlagsIds[i];
        UString m = LangString(id);
        if (m.IsEmpty())
            continue;
        if (f == kpv_ErrorFlags_EncryptedHeadersError) {
            m += " : ";
            AddLangString(m, IDS_EXTRACT_MSG_WRONG_PSW_GUESS);
        }
        if (!s.IsEmpty())
            s.Add_LF();
        s += m;
        errorFlags &= ~f;
    }

    if (errorFlags != 0) {
        char sz[16];
        sz[0] = '0';
        sz[1] = 'x';
        ConvertUInt32ToHex(errorFlags, sz + 2);
        if (!s.IsEmpty())
            s.Add_LF();
        s += sz;
    }

    return s;
}
