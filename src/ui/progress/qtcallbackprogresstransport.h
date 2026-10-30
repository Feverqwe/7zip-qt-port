// Qt worker adapter for the source progress dialog. Worker algorithms stay in their owners.
#pragma once
#include "ui/progress/qtprogresstransport.h"
#include <functional>

class QtCallbackProgressTransport final : public IQtProgressTransport {
    std::function<void()> _cancel;

  public:
    explicit QtCallbackProgressTransport(std::function<void()> cancel);
    HRESULT SetPaused(bool paused) override;
    bool CanPause() const override { return false; }
    void Cancel() override;
};
