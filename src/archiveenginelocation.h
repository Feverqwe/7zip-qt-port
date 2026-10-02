#pragma once
#include <QString>

// Runtime-location glue. Compression and plugin discovery stay in the original
// engine/LoadCodecs; this selects the executable deployed with the application.
QString FindArchiveEngine();
