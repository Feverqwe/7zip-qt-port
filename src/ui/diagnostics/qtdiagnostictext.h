#pragma once
#include "Common/MyString.h"
#include <QCoreApplication>

// Translate an upstream diagnostic before appending paths or other user data.
UString QtDiagnosticText(const char *source);
UString QtDiagnosticText(const char *source, unsigned code);
// CUpdateErrorInfo joins independent ASCII diagnostics with line feeds.
UString QtDiagnosticLines(const AString &source);
// Known archive diagnostic properties contain a reason and sometimes a path.
// Unknown/plugin text remains data and is returned unchanged.
UString QtArchiveDiagnosticText(const UString &source);
