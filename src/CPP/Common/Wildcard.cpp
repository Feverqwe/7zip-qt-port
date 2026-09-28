// Copied from 7-Zip CPP/Common/Wildcard.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// char16_t preserves Windows wchar_t code units on all platforms.
// Qt conversions and character case mapping are boundary adaptations.

#include "Wildcard.h"
#include <QString>
#include <string>

namespace {
// Selection follows Windows 7zFM even on case-sensitive filesystems.
constexpr bool g_CaseSensitive = false;
char16_t MyCharUpper(char16_t c) { return QChar(c).toUpper().unicode(); }

// ? - any char
// * - any char or empty
static bool EnhancedMaskTest(const char16_t *mask, const char16_t *name) {
    for (;;) {
        const char16_t m = *mask;
        const char16_t c = *name;
        if (m == 0)
            return (c == 0);
        if (m == '*') {
            if (EnhancedMaskTest(mask + 1, name))
                return true;
            if (c == 0)
                return false;
        } else {
            if (m == '?') {
                if (c == 0)
                    return false;
            } else if (m != c)
                if (g_CaseSensitive || MyCharUpper(m) != MyCharUpper(c))
                    return false;
            mask++;
        }
        name++;
    }
}

} // namespace

bool DoesWildcardMatchName(const char16_t *mask, const char16_t *name) {
    return EnhancedMaskTest(mask, name);
}

bool DoesWildcardMatchName(QStringView mask, QStringView name) {
    const std::u16string maskString = mask.toString().toStdU16String();
    const std::u16string nameString = name.toString().toStdU16String();
    return DoesWildcardMatchName(maskString.c_str(), nameString.c_str());
}
