#pragma once
#include "Common/MyString.h"
#include <QString>
// Native-file resource service for the source CLang file reader.
QString QtLanguageDirectory();
UString QtEnglishLanguageString(UInt32 id);
// Only Qt/integration text with no upstream resource ID uses this fallback.
QString QtLocalText(const QString &english, const QString &russian);
