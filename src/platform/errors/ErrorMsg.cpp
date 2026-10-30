// Adapted from https://github.com/ip7z/7zip.git, 9128b80e3a471108678e2b3ec3c985861c8d7b0f.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Windows/ErrorMsg.h

#include "Windows/StdAfx.h"

#if !defined(_UNICODE) || !defined(_WIN32)
#include "Common/StringConvert.h"
#endif

#include "Windows/ErrorMsg.h"
#include "localization/qttranslationcatalog.h"
#include "ui/diagnostics/qtdiagnostictext.h"
#include <memory>

#ifdef _WIN32
#if !defined(_UNICODE)
extern bool g_IsNT;
#endif
#endif

namespace NWindows {
namespace NError {

static bool MyFormatMessage(DWORD errorCode, UString &message) {
#ifndef Z7_SFX
    if ((HRESULT)errorCode == MY_HRES_ERROR_INTERNAL_ERROR) {
        message = QtDiagnosticText(QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "Internal Error: The failure in hardware (RAM or CPU), OS or program"));
        return true;
    }
#endif

#ifdef _WIN32

    LPVOID msgBuf;
#ifndef _UNICODE
    if (!g_IsNT) {
        if (::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL, errorCode, 0, (LPTSTR)&msgBuf, 0, NULL) == 0)
            return false;
        const std::unique_ptr<void, decltype(&LocalFree)> buffer(msgBuf, &LocalFree);
        message = GetUnicodeString((LPCTSTR)msgBuf);
    } else
#endif
    {
        if (::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL, errorCode, 0, (LPWSTR)&msgBuf, 0, NULL) == 0)
            return false;
        const std::unique_ptr<void, decltype(&LocalFree)> buffer(msgBuf, &LocalFree);
        message = (LPCWSTR)msgBuf;
    }
    return true;

#else // _WIN32

    AString m;

    const char *s = NULL;

    switch ((Int32)errorCode) {
    // case ERROR_NO_MORE_FILES   : s = "No more files"; break;
    // case ERROR_DIRECTORY       : s = "Error Directory"; break;
    case E_NOTIMPL:
        s = QT_TRANSLATE_NOOP("SevenZipDiagnostics", "E_NOTIMPL : Not implemented");
        break;
    case E_NOINTERFACE:
        s = QT_TRANSLATE_NOOP("SevenZipDiagnostics", "E_NOINTERFACE : No such interface supported");
        break;
    case E_ABORT:
        s = QT_TRANSLATE_NOOP("SevenZipDiagnostics", "E_ABORT : Operation aborted");
        break;
    case E_FAIL:
        s = QT_TRANSLATE_NOOP("SevenZipDiagnostics", "E_FAIL : Unspecified error");
        break;

    case STG_E_INVALIDFUNCTION:
        s = "STG_E_INVALIDFUNCTION";
        break;
    case CLASS_E_CLASSNOTAVAILABLE:
        s = "CLASS_E_CLASSNOTAVAILABLE";
        break;

    case E_OUTOFMEMORY:
        s = QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                              "E_OUTOFMEMORY : Can't allocate required memory");
        break;
    case E_INVALIDARG:
        s = QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                              "E_INVALIDARG : One or more arguments are invalid");
        break;

    // case MY_E_ERROR_NEGATIVE_SEEK : s = "MY_E_ERROR_NEGATIVE_SEEK"; break;
    default:
        break;
    }

    const bool isSourceDiagnostic = (s != NULL);

    /* strerror() for unknown errors still shows message "Unknown error -12345678")
       So we must transfer error codes before strerror() */
    if (!s) {
        if ((errorCode & 0xFFFF0000) == (UInt32)((MY_FACILITY_WRes << 16) | 0x80000000))
            errorCode &= 0xFFFF;
        else if ((errorCode & ((UInt32)1 << 31)))
            return false; // we will show hex error later for that case

        s = strerror((int)errorCode);

        // if (!s)
        {
            m += "errno=";
            m.Add_UInt32(errorCode);
            if (s)
                m += " : ";
        }
    }

    if (s)
        m += s;

    if (isSourceDiagnostic)
        message = QtDiagnosticText(s);
    else
        MultiByteToUnicodeString2(message, m);
    return true;

#endif
}

UString MyFormatMessage(DWORD errorCode) {
    UString m;
    if (!MyFormatMessage(errorCode, m) || m.IsEmpty()) {
        char s[16];
        for (int i = 0; i < 8; i++) {
            unsigned t = errorCode & 0xF;
            errorCode >>= 4;
            s[7 - i] = (char)((t < 10) ? ('0' + t) : ('A' + (t - 10)));
        }
        s[8] = 0;
        m = UString(
            QtTranslatedApplicationText("SevenZipDiagnostics",
                                        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "Error #%1"))
                .arg(QString::fromLatin1(s))
                .toStdWString()
                .c_str());
    } else if (m.Len() >= 2 && m[m.Len() - 1] == 0x0A && m[m.Len() - 2] == 0x0D)
        m.DeleteFrom(m.Len() - 2);
    return m;
}

} // namespace NError
} // namespace NWindows
