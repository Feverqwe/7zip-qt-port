// Adapted from 7-Zip CPP/7zip/UI/Common/ZipRegistry (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "ZipRegistry.h"
#include "Common/StringToInt.h"

inline wchar_t MyCharLower_Ascii(wchar_t c) {
    if (c >= 'A' && c <= 'Z')
        return (wchar_t)(c + 0x20);
    return c;
}

namespace NCompression {
static bool ParseMemUse(const wchar_t *s, CMemUse &mu) {
    mu.Clear();

    bool percentMode = false;
    {
        const wchar_t c = *s;
        if (MyCharLower_Ascii(c) == 'p') {
            percentMode = true;
            s++;
        }
    }
    const wchar_t *end;
    UInt64 number = ConvertStringToUInt64(s, &end);
    if (end == s)
        return false;

    wchar_t c = *end;

    if (percentMode) {
        if (c != 0)
            return false;
        mu.IsPercent = true;
        mu.Val = number;
        return true;
    }

    if (c == 0) {
        mu.Val = number;
        return true;
    }

    c = MyCharLower_Ascii(c);

    const wchar_t c1 = end[1];

    if (c == '%') {
        if (c1 != 0)
            return false;
        mu.IsPercent = true;
        mu.Val = number;
        return true;
    }

    if (c == 'b') {
        if (c1 != 0)
            return false;
        mu.Val = number;
        return true;
    }

    if (c1 != 0)
        if (MyCharLower_Ascii(c1) != 'b' || end[2] != 0)
            return false;

    unsigned numBits;
    switch (c) {
    case 'k':
        numBits = 10;
        break;
    case 'm':
        numBits = 20;
        break;
    case 'g':
        numBits = 30;
        break;
    case 't':
        numBits = 40;
        break;
    default:
        return false;
    }
    if (number >= ((UInt64)1 << (64 - numBits)))
        return false;
    mu.Val = number << numBits;
    return true;
}

void CMemUse::Parse(const QString &s) { IsDefined = ParseMemUse(s.toStdWString().c_str(), *this); }

} // namespace NCompression
