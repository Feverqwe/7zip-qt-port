// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelListNotify.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// The integer formatting algorithm is copied; QString replaces UString.
#include "Panel.h"

#define INT_TO_STR_SPEC(v)                                                                         \
    while (v >= 10) {                                                                              \
        temp[i++] = (Byte)('0' + (unsigned)(v % 10));                                              \
        v /= 10;                                                                                   \
    }                                                                                              \
    *s++ = (Byte)('0' + (unsigned)v);

static void ConvertSizeToString(UInt64 val, wchar_t *s) noexcept {
    Byte temp[32];
    unsigned i = 0;
    if (val <= (UInt32)0xFFFFFFFF) {
        UInt32 val32 = (UInt32)val;
        INT_TO_STR_SPEC(val32)
    } else {
        INT_TO_STR_SPEC(val)
    }
    if (i < 3) {
        if (i != 0) {
            *s++ = temp[(size_t)i - 1];
            if (i == 2)
                *s++ = temp[0];
        }
        *s = 0;
        return;
    }
    unsigned r = i % 3;
    if (r != 0) {
        s[0] = temp[--i];
        if (r == 2)
            s[1] = temp[--i];
        s += r;
    }
    do {
        s[0] = ' ';
        s[1] = temp[(size_t)i - 1];
        s[2] = temp[(size_t)i - 2];
        s[3] = temp[(size_t)i - 3];
        s += 4;
    } while (i -= 3);
    *s = 0;
}

#undef INT_TO_STR_SPEC

QString ConvertSizeToString(UInt64 value) {
    wchar_t s[32];
    ConvertSizeToString(value, s);
    return QString::fromWCharArray(s);
}
