#pragma once
#include "dialogs/listview/ListViewDialog.h"
#include "filemanager/panels/Panel.h"

// Inspect the public source collector without invoking an OS property sheet.
class PropertyProjection final : public CListViewDialog {
  public:
    PropertyProjection(const CPanel &panel, const CRecordVector<UInt32> &indices)
        : CListViewDialog() {
        panel.Properties(indices, *this);
        OnInit();
        setObjectName("selectedPropertiesDialog");
    }
};
