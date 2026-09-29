// Qt metadata snapshot adapter for the unchanged upstream folder contract.
#pragma once
#include "CPP/7zip/UI/FileManager/Panel.h"

class QtPanelFolder final : public IFolderFolder, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderFolder)
  public:
    enum class PropertyCatalog { CustomPresentation, ArchiveTransport };
    QtPanelFolder(const QString &prefix, const CObjectVector<CPanelItem> &items,
                  PropertyCatalog catalog = PropertyCatalog::CustomPresentation);
    Z7_IFACE_COM7_IMP(IFolderFolder)

  private:
    QString prefix_;
    CObjectVector<CPanelItem> items_;
    UStringVector propertyNames_;
    PropertyCatalog catalog_;
};
