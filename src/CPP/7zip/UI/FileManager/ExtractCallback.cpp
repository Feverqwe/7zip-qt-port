// Adapted from 7-Zip CPP/7zip/UI/FileManager/ExtractCallback.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Complete error flag resource array and GetOpenArcErrorMessage body only.
#include "ExtractCallback.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/PropID.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "Common/IntToString.h"
#include "FormatUtils.h"
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

// Complete upstream SetExtractErrorMessage body; current resource provider uses
// the original non-Z7_LANG fallback. Language-file lookup remains P1-14.
void SetExtractErrorMessage(Int32 opRes, Int32 encrypted, const wchar_t *fileName, UString &s) {
    s.Empty();

    if (opRes == NArchive::NExtract::NOperationResult::kOK)
        return;

#ifndef Z7_SFX
    UINT messageID = 0;
#endif
    UINT id = 0;

    switch (opRes) {
    case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
#ifndef Z7_SFX
        messageID = IDS_EXTRACT_MESSAGE_UNSUPPORTED_METHOD;
#endif
        id = IDS_EXTRACT_MSG_UNSUPPORTED_METHOD;
        break;
    case NArchive::NExtract::NOperationResult::kDataError:
#ifndef Z7_SFX
        messageID =
            encrypted ? IDS_EXTRACT_MESSAGE_DATA_ERROR_ENCRYPTED : IDS_EXTRACT_MESSAGE_DATA_ERROR;
#endif
        id = IDS_EXTRACT_MSG_DATA_ERROR;
        break;
    case NArchive::NExtract::NOperationResult::kCRCError:
#ifndef Z7_SFX
        messageID =
            encrypted ? IDS_EXTRACT_MESSAGE_CRC_ERROR_ENCRYPTED : IDS_EXTRACT_MESSAGE_CRC_ERROR;
#endif
        id = IDS_EXTRACT_MSG_CRC_ERROR;
        break;
    case NArchive::NExtract::NOperationResult::kUnavailable:
        id = IDS_EXTRACT_MSG_UNAVAILABLE_DATA;
        break;
    case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
        id = IDS_EXTRACT_MSG_UEXPECTED_END;
        break;
    case NArchive::NExtract::NOperationResult::kDataAfterEnd:
        id = IDS_EXTRACT_MSG_DATA_AFTER_END;
        break;
    case NArchive::NExtract::NOperationResult::kIsNotArc:
        id = IDS_EXTRACT_MSG_IS_NOT_ARC;
        break;
    case NArchive::NExtract::NOperationResult::kHeadersError:
        id = IDS_EXTRACT_MSG_HEADERS_ERROR;
        break;
    case NArchive::NExtract::NOperationResult::kWrongPassword:
        id = IDS_EXTRACT_MSG_WRONG_PSW_CLAIM;
        break;
        /*
        default:
          messageID = IDS_EXTRACT_MESSAGE_UNKNOWN_ERROR;
          break;
        */
    }

    UString msg;

#ifndef Z7_SFX
    UString msgOld;
#ifdef Z7_LANG
    if (id != 0)
        LangString_OnlyFromLangFile(id, msg);
    if (messageID != 0 && msg.IsEmpty())
        LangString_OnlyFromLangFile(messageID, msgOld);
#else
    (void)messageID;
#endif
    if (msg.IsEmpty() && !msgOld.IsEmpty())
        s = MyFormatNew(msgOld, fileName);
    else
#endif
    {
        if (msg.IsEmpty() && id != 0)
            LangString(id, msg);
        if (!msg.IsEmpty())
            s += msg;
        else {
            s += "Error #";
            s.Add_UInt32((UInt32)opRes);
        }

        if (encrypted && opRes != NArchive::NExtract::NOperationResult::kWrongPassword) {
            // s += " : ";
            // AddLangString(s, IDS_EXTRACT_MSG_ENCRYPTED);
            s += " : ";
            AddLangString(s, IDS_EXTRACT_MSG_WRONG_PSW_GUESS);
        }
        s += " : ";
        s += fileName;
    }
}
