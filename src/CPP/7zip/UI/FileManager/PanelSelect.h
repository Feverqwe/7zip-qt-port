// Selection rules extracted from 7-Zip CPanel (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include <QString>
#include <vector>

struct CPanelSelectionItem {
    QString Name;
    bool IsFolder = false;
    bool Selected = false;
};

// One snapshot of the displayed items; the presentation adapter owns focus
// and publishes Selected values back to its selection model.
class CPanelSelection {
  public:
    std::vector<CPanelSelectionItem> Items;
    void SelectSpec(QStringView mask, bool selectMode);
    void SelectByType(int focusedItem, bool selectMode);
    void SelectAll(bool selectMode);
    void InvertSelection();
};
