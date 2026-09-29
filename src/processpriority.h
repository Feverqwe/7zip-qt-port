// Native platform service for the source background transition and 7zz transport.
#pragma once
#include "Common/MyWindows.h"

namespace NProcessPriority {
// Zero selects the current process. A nonzero ID must be an owned live process.
HRESULT GetBackground(UInt32 processId, bool &background);
HRESULT SetBackground(UInt32 processId, bool background);
} // namespace NProcessPriority
