#pragma once
#ifdef _WIN32
#include "Common/MyTypes.h"
#include "Common/MyVector.h"
#include "Common/MyWindows.h"
#include <functional>
#include <memory>

class CPanel;
class QMenu;

// Retains the original shell COM object and HMENU for one Qt menu generation.
class QtNativeShellMenu final {
    struct State;
    std::unique_ptr<State> _state;

  public:
    QtNativeShellMenu();
    ~QtNativeShellMenu();
    bool Create(CPanel &panel, bool extended, const CRecordVector<UInt32> &indices, QMenu &menu,
                std::function<void(unsigned)> dispatch);
    bool CanInvoke(unsigned id) const;
    HRESULT Invoke(unsigned offset);
};
#endif
