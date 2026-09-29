// Worker-to-GUI process service used by native archive callbacks.
#pragma once
#include "Common/MyWindows.h"
#include <functional>
class ArchiveBackend;
class CProgressSync;
class IUnknown;
HRESULT RunArchiveBackendRequest(ArchiveBackend &backend, const std::function<void()> &start,
                                 IUnknown *passwordCallback, CProgressSync *sync);
