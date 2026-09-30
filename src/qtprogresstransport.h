// Narrow capabilities for source dialogs whose worker uses an external engine.
#pragma once
#include "Common/MyWindows.h"
#include <memory>

class CProgressDialog;
class IQtProgressTransport {
  public:
    virtual ~IQtProgressTransport() = default;
    virtual HRESULT SetPaused(bool paused) = 0;
    virtual HRESULT SetBackground(bool background) = 0;
    virtual bool CanPause() const { return true; }
    virtual void Cancel() {}
};

class IQtProgressServices {
  public:
    virtual ~IQtProgressServices() = default;
    virtual std::unique_ptr<IQtProgressTransport>
    CreateProgressTransport(CProgressDialog &dialog) = 0;
};
