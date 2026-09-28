#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "qtpanelfolder.h"
#include <QTableWidget>
#include <QtTest>

class PanelFolderTests : public QObject {
    Q_OBJECT
  private slots:
    void metadataSnapshotOwnsTypedProperties() {
        CObjectVector<CPanelItem> items;
        CPanelItem item;
        item.RelPath = QStringLiteral("папка с пробелом/файл 😀.txt");
        item.SizeDefined = true;
        item.Size = 0x100000000ULL;
        item.Properties.Add({UString(L"Имя"), UString(L"файл 😀.txt")});
        items.Add(item);
        CMyComPtr<IFolderFolder> folder = new QtPanelFolder("archive.7z / ", items);
        items[0].RelPath = "changed";
        items[0].Size = 1;
        NWindows::NCOM::CPropVariant prop;
        QCOMPARE(folder->GetProperty(0, kpidName, &prop), S_OK);
        QCOMPARE(ToQString(UString(prop.bstrVal)), QStringLiteral("файл 😀.txt"));
        QCOMPARE(folder->GetProperty(0, kpidPrefix, &prop), S_OK);
        QCOMPARE(ToQString(UString(prop.bstrVal)), QStringLiteral("папка с пробелом/"));
        QCOMPARE(folder->GetProperty(0, kpidSize, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_UI8));
        QCOMPARE(prop.uhVal.QuadPart, UInt64(0x100000000ULL));
        QCOMPARE(folder->GetProperty(0, kpidIsDir, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_BOOL));
        QCOMPARE(prop.boolVal, VARIANT_BOOL(VARIANT_FALSE));
        QCOMPARE(folder->GetProperty(1, kpidName, &prop), E_INVALIDARG);
        QCOMPARE(folder->GetProperty(0, kpidCRC, &prop), E_NOTIMPL);
        QCOMPARE(ToQString(GetFolderPath(folder)), QString("archive.7z / "));
        CMyComPtr<IFolderFolder> child;
        QCOMPARE(folder->BindToFolder(UInt32(0), &child), E_NOTIMPL);
        QVERIFY(!child);
        QCOMPARE(folder->BindToParentFolder(&child), E_NOTIMPL);
        QVERIFY(!child);
    }

    void panelUsesInterfaceAfterExternalOwnerRelease() {
        CObjectVector<CPanelItem> items;
        items.Add({QStringLiteral("внутри/файл.txt"), false, true, 12345});
        CMyComPtr<IFolderFolder> folder = new QtPanelFolder("/folder/", items);
        CPanel panel(folder);
        folder.Release();
        CRecordVector<UInt32> indices;
        indices.Add(0);
        QCOMPARE(panel.GetItemsInfoString(indices),
                 QStringLiteral("Файлов: 1    ( 12 345 байт )\n\n/folder/\n  внутри/файл.txt"));
    }

    void missingMetadataValueIsEmptyInsteadOfAnotherItemsValue() {
        CObjectVector<CPanelItem> items;
        CPanelItem first;
        first.Properties.Add({UString(L"A"), UString(L"value A")});
        CPanelItem second;
        second.Properties.Add({UString(L"B"), UString(L"value B")});
        items.Add(first);
        items.Add(second);
        CMyComPtr<IFolderFolder> folder = new QtPanelFolder("", items);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfProperties(&count), S_OK);
        QCOMPARE(count, 2u);
        CMyComBSTR name;
        PROPID id;
        VARTYPE type;
        QCOMPARE(folder->GetPropertyInfo(0, &name, &id, &type), S_OK);
        QCOMPARE(ToQString(UString(name)), QString("A"));
        NWindows::NCOM::CPropVariant prop;
        QCOMPARE(folder->GetProperty(1, id, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(folder->GetProperty(0, kpidSize, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_EMPTY));
        UInt64 size = 99;
        QVERIFY(!ConvertPropVariantToUInt64(prop, size));
        prop = UInt32(0);
        QVERIFY(ConvertPropVariantToUInt64(prop, size));
        QCOMPARE(size, UInt64(0));
        prop = true;
        bool threw = false;
        try {
            ConvertPropVariantToUInt64(prop, size);
        } catch (int error) {
            QCOMPARE(error, 151199);
            threw = true;
        }
        QVERIFY(threw);
    }

    void releaseClearsBoundListAndSelectedState() {
        CObjectVector<CPanelItem> items;
        items.Add({"file.txt", false, true, 1});
        CPanel panel("/folder/", items);
        QTableWidget list(1, 1);
        list.setItem(0, 0, new QTableWidgetItem("file.txt"));
        list.selectRow(0);
        panel.AttachSelectionView(&list);
        CSelectedState before;
        panel.SaveSelectedState(before);
        QCOMPARE(before.SelectedNames.Size(), 1u);
        panel.ReleaseFolder();
        QCOMPARE(list.rowCount(), 0);
        CSelectedState after;
        panel.SaveSelectedState(after);
        QCOMPARE(after.FocusedItem, -1);
        QCOMPARE(after.SelectedNames.Size(), 0u);
        panel.ReleaseFolder();
        QCOMPARE(list.rowCount(), 0);
    }
};

QTEST_MAIN(PanelFolderTests)
#include "panel_folder_tests.moc"
