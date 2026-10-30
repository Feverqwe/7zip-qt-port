#pragma once
#include "settings/ViewSettings.h"
#include <limits>

class CApp;

namespace NQtFileManager {
// Win32 startup values cross the narrow presentation boundary unchanged.
namespace NShowCommand {
constexpr int Hide = 0;
constexpr int Normal = 1;
constexpr int Minimized = 2;
constexpr int Maximized = 3;
constexpr int NoActivate = 4;
constexpr int Show = 5;
constexpr int Minimize = 6;
constexpr int MinNoActivate = 7;
constexpr int ShowNoActivate = 8;
constexpr int Restore = 9;
constexpr int Default = 10;
constexpr int ForceMinimize = 11;
} // namespace NShowCommand
constexpr int WindowUseDefault = std::numeric_limits<int>::min();
} // namespace NQtFileManager

struct QtFileManagerPlacement {
    CWindowRect normalPosition;
    unsigned showCmd = NQtFileManager::NShowCommand::Normal;
};

class IQtFileManagerInitServices {
  public:
    virtual ~IQtFileManagerInitServices() = default;
    virtual bool Create(int x, int y, int width, int height, bool panelsDefined,
                        UInt32 splitterPos) = 0;
    virtual bool GetPlacement(QtFileManagerPlacement &placement) const = 0;
    virtual void SetPlacement(const QtFileManagerPlacement &placement) = 0;
    virtual void Show(int command) = 0;
};

namespace NQtFileManager {
bool InitInstance(int nCmdShow, CApp &g_App, IQtFileManagerInitServices &wnd,
                  const QString &settingsPath);
}
