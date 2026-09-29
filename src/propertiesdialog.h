#pragma once

#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
class CPanel;

// Qt dialog construction; the bound source CPanel owns metadata and totals.
class PropertiesDialog : public CListViewDialog {
  public:
    explicit PropertiesDialog(const CPanel &panel, const CRecordVector<UInt32> &indices,
                              QWidget *parent = nullptr);
};
