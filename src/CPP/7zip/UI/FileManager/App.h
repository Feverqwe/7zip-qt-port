// Adapted from 7-Zip CPP/7zip/UI/FileManager/App.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "AppState.h"
#include "Panel.h"

const unsigned kNumPanelsMax = 2;

class CApp {
  public:
    unsigned NumPanels = 2;
    unsigned LastFocusedPanel = 0;
    CAppState AppState;
    CPanel Panels[kNumPanelsMax];

    unsigned GetFocusedPanelIndex() const { return LastFocusedPanel; }
    bool IsPanelVisible(unsigned index) const { return NumPanels > 1 || index == LastFocusedPanel; }
    CPanel &GetFocusedPanel() { return Panels[GetFocusedPanelIndex()]; }
    void SetFocusToLastItem() { Panels[LastFocusedPanel].SetFocusToLastRememberedItem(); }
};
