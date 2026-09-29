#include "propertiesdialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"

PropertiesDialog::PropertiesDialog(const CPanel &panel, const CRecordVector<UInt32> &indices,
                                   QWidget *parent)
    : CListViewDialog(parent) {
    panel.Properties(indices, *this);
    OnInit();
    setObjectName("selectedPropertiesDialog");
}
