#pragma once
#include "7zip/IStream.h"

// Retain the copied codec image and owned input beside a native item stream.
HRESULT CreateArchiveItemStream(IUnknown *resource, UInt32 index, ISequentialInStream **stream);
