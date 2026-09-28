// Adapted from 7-Zip CPP/Common/Wildcard.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// QStringView and QChar replace UString and wchar_t for the Qt UI.

#include "Wildcard.h"

// ? - any char
// * - any char or empty
static bool EnhancedMaskTest(QStringView mask, QStringView name) {
    for (;;) {
        const QChar m = mask.isEmpty() ? QChar() : mask.front();
        const QChar c = name.isEmpty() ? QChar() : name.front();
        if (m.isNull())
            return c.isNull();
        if (m == QLatin1Char('*')) {
            if (EnhancedMaskTest(mask.mid(1), name))
                return true;
            if (c.isNull())
                return false;
        } else {
            if (m == QLatin1Char('?')) {
                if (c.isNull())
                    return false;
            } else if (m != c && m.toUpper() != c.toUpper())
                return false;
            mask = mask.mid(1);
        }
        name = name.mid(1);
    }
}

bool DoesWildcardMatchName(QStringView mask, QStringView name) {
    return EnhancedMaskTest(mask, name);
}
