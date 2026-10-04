#pragma once
#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"

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
