#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "qtlocationfolder.h"
#include "qtpanelfolder.h"

#include <QTest>

using NWindows::NCOM::CPropVariant;

class ArchivePropertyCatalogTests : public QObject {
    Q_OBJECT

  private slots:
    void transportCatalogOwnsNativeTypesAndFullUnicodePaths() {
        CObjectVector<CPanelItem> items;
        CPanelItem item;
        item.RelPath = QStringLiteral("папка й/файл 😀 с пробелом.txt");
        item.SizeDefined = true;
        item.Size = 0xFFFFFFFFFFFFFFFFULL;
        item.PackSize = 0;
        // UI custom fields cannot redefine the archive engine metadata catalog.
        item.Properties.Add({UString(L"formatted UI name"), UString(L"formatted UI value")});
        items.Add(item);
        CMyComPtr<IFolderFolder> folder =
            new QtPanelFolder("", items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        items.Clear();
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfProperties(&count), S_OK);
        QCOMPARE(count, 3u);
        const PROPID expectedIds[] = {kpidPath, kpidSize, kpidPackSize};
        const VARTYPE expectedTypes[] = {VT_BSTR, VT_UI8, VT_UI8};
        for (UInt32 i = 0; i < count; ++i) {
            CMyComBSTR name;
            PROPID id;
            VARTYPE type;
            QCOMPARE(folder->GetPropertyInfo(i, &name, &id, &type), S_OK);
            QVERIFY(!name);
            QCOMPARE(id, expectedIds[i]);
            QCOMPARE(type, expectedTypes[i]);
        }
        CPropVariant prop;
        QCOMPARE(folder->GetProperty(0, kpidPath, &prop), S_OK);
        QCOMPARE(QString::fromWCharArray(prop.bstrVal), item.RelPath);
        QCOMPARE(folder->GetProperty(0, kpidSize, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_UI8));
        QCOMPARE(prop.uhVal.QuadPart, UInt64(0xFFFFFFFFFFFFFFFFULL));
        QCOMPARE(folder->GetProperty(0, kpidPackSize, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_UI8));
        QCOMPARE(prop.uhVal.QuadPart, UInt64(0));
        CMyComBSTR name;
        PROPID id;
        VARTYPE type;
        QCOMPARE(folder->GetPropertyInfo(count, &name, &id, &type), E_INVALIDARG);
        QCOMPARE(folder->GetProperty(1, kpidPath, &prop), E_INVALIDARG);
        QCOMPARE(folder->GetProperty(0, kpidMTime, &prop), E_NOTIMPL);
    }

    void knownCatalogDoesNotFabricateAbsentValues() {
        CObjectVector<CPanelItem> items;
        CPanelItem unknown;
        unknown.RelPath = "unknown";
        unknown.PackSizeDefined = false;
        items.Add(unknown);
        CMyComPtr<IFolderFolder> folder =
            new QtPanelFolder("", items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        CPropVariant prop(UInt64(123));
        QCOMPARE(folder->GetProperty(0, kpidSize, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_EMPTY));
        prop = UInt64(123);
        QCOMPARE(folder->GetProperty(0, kpidPackSize, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_EMPTY));
        CObjectVector<CPanelItem> empty;
        CMyComPtr<IFolderFolder> noItems =
            new QtPanelFolder("", empty, QtPanelFolder::PropertyCatalog::ArchiveTransport);
        UInt32 count;
        QCOMPARE(noItems->GetNumberOfProperties(&count), S_OK);
        QCOMPARE(count, 3u);
        QCOMPARE(noItems->GetProperty(0, kpidSize, &prop), E_INVALIDARG);
    }

    void archiveFactoryPublishesTypedProviderWithoutChangingLegacyCatalog() {
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(CreateArchiveFolder({ArchiveEntry{QStringLiteral("папка/файл.txt"), -1, 0}},
                                     UString(L"7z"), &folder),
                 S_OK);
        UInt32 count;
        QCOMPARE(folder->GetNumberOfProperties(&count), S_OK);
        QVERIFY(count >= 3); // Folder-side source expansion is the following substep.
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(folder->BindToFolder(UInt32(0), &child), S_OK);
        CPropVariant prop;
        QCOMPARE(child->GetProperty(0, kpidSize, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(child->GetProperty(0, kpidPackSize, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_UI8));
        QCOMPARE(prop.uhVal.QuadPart, UInt64(0));
        CObjectVector<CPanelItem> custom;
        CPanelItem ui;
        ui.Properties.Add({UString(L"Custom й"), UString(L"value")});
        custom.Add(ui);
        CMyComPtr<IFolderFolder> legacy = new QtPanelFolder("", custom);
        QCOMPARE(legacy->GetNumberOfProperties(&count), S_OK);
        QCOMPARE(count, 1u);
        CMyComBSTR name;
        PROPID id;
        VARTYPE type;
        QCOMPARE(legacy->GetPropertyInfo(0, &name, &id, &type), S_OK);
        QCOMPARE(QString::fromWCharArray(name), QStringLiteral("Custom й"));
        QCOMPARE(type, VARTYPE(VT_BSTR));
    }
};

QTEST_GUILESS_MAIN(ArchivePropertyCatalogTests)
#include "archive_property_catalog_tests.moc"
