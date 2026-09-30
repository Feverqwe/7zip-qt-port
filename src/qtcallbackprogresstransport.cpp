#include "qtcallbackprogresstransport.h"
#include "processpriority.h"
#include <utility>

QtCallbackProgressTransport::QtCallbackProgressTransport(std::function<void()> cancel)
    : _cancel(std::move(cancel)) {}

HRESULT QtCallbackProgressTransport::SetPaused(bool) { return E_NOTIMPL; }

HRESULT QtCallbackProgressTransport::SetBackground(bool background) {
    return NProcessPriority::SetBackground(0, background);
}

void QtCallbackProgressTransport::Cancel() {
    if (_cancel)
        _cancel();
}
