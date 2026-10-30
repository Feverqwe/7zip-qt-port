#pragma once
#include <QString>

// Runtime-location glue. Compression and plugin discovery stay in the original
// engine/LoadCodecs; this selects the native module deployed with the application.
QString FindArchiveModule(const QString &location = {});
