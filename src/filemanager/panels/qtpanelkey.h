#pragma once
#include <Qt>

// Decoded Qt list notification, replacing NMLVKEYDOWN/GetKeyState.
struct QtPanelKeyDown {
    int VKey;
    bool Alt;
    bool Control;
    bool RightControl;
    bool Shift;
};
