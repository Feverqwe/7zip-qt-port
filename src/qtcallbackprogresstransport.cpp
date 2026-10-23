#include "qtcallbackprogresstransport.h"
#include <utility>

QtCallbackProgressTransport::QtCallbackProgressTransport(std::function<void()> cancel)
    : _cancel(std::move(cancel)) {}

HRESULT QtCallbackProgressTransport::SetPaused(bool) { return E_NOTIMPL; }

void QtCallbackProgressTransport::Cancel() {
    if (_cancel)
        _cancel();
}
