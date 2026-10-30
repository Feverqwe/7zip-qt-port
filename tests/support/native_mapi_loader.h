#pragma once
#include <QString>
#include <windows.h>

// A failed private-provider identity check stops before the mail workflow.
HMODULE LoadPrivateMAPIProvider(const QString &path);
