// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelDrag.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Partial owner prerequisite: the CDropTarget model/drag workflow remains P0-08.
#include "App.h"

void CApp::SetFocusedPanel(unsigned index) {
    LastFocusedPanel = index;
    // Original CDropTarget::TargetPanelIndex synchronization is mandatory when
    // the second panel/drop-target owner is ported; Qt currently has one target.
}
