#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "layout_inspection.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QScopedValueRollback>
#include <QTest>
#include <QTreeWidget>

using NWindows::NCOM::CPropVariant;

struct TestProperty {
    PROPID Id;
    UString Name;
    CPropVariant Value;
    HRESULT InfoResult = S_OK;
    HRESULT ValueResult = S_OK;
};

// Typed public folder contract fixture; production internals are not exposed.
class PropertyFolder final : public IFolderFolder, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderFolder)
  public:
    explicit PropertyFolder(const CObjectVector<TestProperty> &properties)
        : properties_(properties) {}
    Z7_IFACE_COM7_IMP(IFolderFolder)

  private:
    CObjectVector<TestProperty> properties_;
};

Z7_COM7F_IMF(PropertyFolder::LoadItems()) { return S_OK; }
Z7_COM7F_IMF(PropertyFolder::GetNumberOfItems(UInt32 *count)) {
    *count = 1;
    return S_OK;
}
Z7_COM7F_IMF(PropertyFolder::GetNumberOfProperties(UInt32 *count)) {
    *count = properties_.Size();
    return S_OK;
}
Z7_COM7F_IMF(PropertyFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id, VARTYPE *type)) {
    if (index >= properties_.Size())
        return E_INVALIDARG;
    const auto &entry = properties_[index];
    if (entry.InfoResult != S_OK)
        return entry.InfoResult;
    *name = entry.Name.IsEmpty() ? nullptr : ::SysAllocStringLen(entry.Name, entry.Name.Len());
    *id = entry.Id;
    *type = entry.Value.vt;
    return S_OK;
}
Z7_COM7F_IMF(PropertyFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (index != 0)
        return E_INVALIDARG;
    FOR_VECTOR(i, properties_) {
        const auto &entry = properties_[i];
        if (entry.Id != id)
            continue;
        if (entry.ValueResult != S_OK)
            return entry.ValueResult;
        CPropVariant prop(entry.Value);
        return prop.Detach(value);
    }
    return E_NOTIMPL;
}
Z7_COM7F_IMF(PropertyFolder::GetFolderProperty(PROPID, PROPVARIANT *)) { return E_NOTIMPL; }
Z7_COM7F_IMF(PropertyFolder::BindToFolder(UInt32, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(PropertyFolder::BindToFolder(const wchar_t *, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(PropertyFolder::BindToParentFolder(IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}

class PanelPropertyTests : public QObject {
    Q_OBJECT
    static QString value(const CListViewDialog &dialog, const QString &name) {
        FOR_VECTOR(i, dialog.Strings)
        if (QString::fromWCharArray(dialog.Strings[i]) == name)
            return QString::fromWCharArray(dialog.Values[i]);
        return QString();
    }
    static void render(const CObjectVector<TestProperty> &properties, CListViewDialog &dialog) {
        CMyComPtr<IFolderFolder> folder = new PropertyFolder(properties);
        CPanel panel(folder);
        CRecordVector<UInt32> selected;
        selected.Add(0);
        panel.Properties(selected, true, dialog);
    }

  private slots:
    void typedCatalogUsesSourceNamesAndFormatting() {
        QScopedValueRollback<bool> utc(g_Timestamp_Show_UTC, true);
        CObjectVector<TestProperty> props;
        props.Add({kpidName, L"wrong built-in caption", CPropVariant(L"файл й 😀.txt")});
        props.Add({kpidSize, L"", CPropVariant(UInt64(0xFFFFFFFFFFFFFFFFULL))});
        props.Add({kpidPackSize, L"", CPropVariant(UInt32(12345))});
        CPropVariant count;
        count.vt = VT_UI2;
        count.uiVal = 1234;
        props.Add({kpidNumSubFiles, L"", count});
        props.Add({kpidCRC, L"", CPropVariant(UInt32(0x00deadbe))});
        props.Add({kpidIsDir, L"", CPropVariant(false)});
        CPropVariant date;
        date.vt = VT_FILETIME;
        const UInt64 ticks = UInt64(11644473600ULL) * 10000000 + 1234567;
        date.filetime.dwLowDateTime = UInt32(ticks);
        date.filetime.dwHighDateTime = UInt32(ticks >> 32);
        date.wReserved1 = k_PropVar_TimePrec_1ns;
        date.wReserved2 = 89;
        props.Add({kpidMTime, L"", date});
        props.Add({999, L"Свойство й", CPropVariant(L"значение с пробелом")});
        props.Add({123456, L"", CPropVariant(UInt32(42))});
        CListViewDialog dialog;
        render(props, dialog);
        QCOMPARE(value(dialog, "Имя"), QStringLiteral("файл й 😀.txt"));
        QCOMPARE(value(dialog, "Размер"), QString("18 446 744 073 709 551 615"));
        QCOMPARE(value(dialog, "Сжатый размер"), QString("12 345"));
        QCOMPARE(value(dialog, "Файлов"), QString("1 234"));
        QCOMPARE(value(dialog, "CRC"), QString("00DEADBE"));
        QCOMPARE(value(dialog, "Папка"), QString("-"));
        QCOMPARE(value(dialog, "Изменён"), QString("1970-01-01 00:00:00.123456789Z"));
        QCOMPARE(value(dialog, "Свойство й"), QStringLiteral("значение с пробелом"));
        QCOMPARE(value(dialog, "123456"), QString("42"));
    }

    void emptyValuesAndFailedCatalogEntriesAreSkipped() {
        CObjectVector<TestProperty> props;
        props.Add({kpidSize, L"", CPropVariant()});
        props.Add({kpidComment, L"", CPropVariant(L"")});
        props.Add({kpidMTime, L"", CPropVariant(FILETIME{})});
        props.Add({kpidErrorFlags, L"", CPropVariant(UInt32(0))});
        props.Add({kpidWarningFlags, L"", CPropVariant()});
        props.Add({999, L"bad info", CPropVariant(L"ignored"), E_FAIL});
        props.Add({998, L"bad value", CPropVariant(L"ignored"), S_OK, E_FAIL});
        props.Add({kpidPackSize, L"", CPropVariant(UInt64(0))});
        CListViewDialog dialog;
        render(props, dialog);
        QCOMPARE(dialog.Strings.Size(), 3u); // Defined packed zero, separator and path.
        QCOMPARE(value(dialog, "Сжатый размер"), QString("0"));
        QVERIFY(value(dialog, "Размер").isNull());
        QVERIFY(value(dialog, "Изменён").isNull());
        QVERIFY(value(dialog, "Ошибки").isNull());
    }

    void errorFlagsAndExpectedTypeWarningKeepSourceBranches() {
        CObjectVector<TestProperty> props;
        props.Add(
            {kpidErrorFlags, L"", CPropVariant(UInt64(kpv_ErrorFlags_EncryptedHeadersError))});
        props.Add(
            {kpidWarningFlags, L"", CPropVariant(UInt32(0x80000000 | kpv_ErrorFlags_CrcError))});
        props.Add({kpidErrorType, L"", CPropVariant(L"7z")});
        CListViewDialog dialog;
        render(props, dialog);
        QCOMPARE(value(dialog, "Ошибки"), QStringLiteral("Ошибка заголовков : Неверный пароль?"));
        QCOMPARE(value(dialog, "Предупреждения"), QStringLiteral("Ошибка CRC\n0x80000000"));
        QCOMPARE(value(dialog, "Open WARNING:"),
                 QString("Cannot open the file as expected archive type"));
        QCOMPARE(value(dialog, "Тип ошибки"), QString("7z"));
        CObjectVector<TestProperty> invalid;
        invalid.Add({kpidErrorFlags, L"", CPropVariant(L"invalid")});
        CListViewDialog badDialog;
        bool threw = false;
        try {
            render(invalid, badDialog);
        } catch (int error) {
            QCOMPARE(error, 151199);
            threw = true;
        }
        QVERIFY(threw);
    }

    void aggregateCountsUseTheSameGroupedPropertyFormatter() {
        CObjectVector<CPanelItem> items;
        CPanelItem folder;
        folder.RelPath = "folder й";
        folder.IsFolder = true;
        folder.SizeDefined = true;
        folder.Size = 10000;
        folder.PackSize = 2000;
        folder.NumSubDirs = 1233;
        folder.NumSubFiles = 5677;
        items.Add(folder);
        CPanelItem file;
        file.RelPath = "file with spaces";
        file.SizeDefined = true;
        file.Size = 345;
        file.PackSize = 34;
        items.Add(file);
        CPanel panel("/source/", items);
        CRecordVector<UInt32> indices;
        indices.Add(0);
        indices.Add(1);
        CListViewDialog dialog;
        panel.Properties(indices, true, dialog);
        QCOMPARE(value(dialog, "Папок"), QString("1 234"));
        QCOMPARE(value(dialog, "Файлов"), QString("5 678"));
        QCOMPARE(value(dialog, "Размер"), QString("10 345"));
        QCOMPARE(value(dialog, "Сжатый размер"), QString("2 034"));
    }

    void typedValuesGeometry_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(540, 360);
        QTest::newRow("compact") << QSize(360, 240);
    }

    void typedValuesGeometry() {
        QFETCH(QSize, size);
        CObjectVector<TestProperty> props;
        const auto path = QStringLiteral("папка й/файл 😀 с пробелом/").repeated(80).toStdWString();
        props.Add({kpidName, L"", CPropVariant(path.c_str())});
        props.Add({kpidSize, L"", CPropVariant(UInt64(0xFFFFFFFFFFFFFFFFULL))});
        props.Add(
            {kpidErrorFlags, L"", CPropVariant(UInt32(0x80000000 | kpv_ErrorFlags_CrcError))});
        CListViewDialog dialog;
        render(props, dialog);
        QVERIFY(dialog.OnInit());
        dialog.setObjectName("selectedPropertiesDialog");
        dialog.resize(size);
        dialog.show();
        QApplication::processEvents();
        auto *list = dialog.findChild<QTreeWidget *>("propertiesList");
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(list && buttons);
        QCOMPARE(list->columnCount(), 2);
        QVERIFY(!list->header()->isHidden());
        QVERIFY(UiInspection::fullyVisible(*list, dialog));
        QVERIFY(UiInspection::fullyVisible(*list->header(), dialog));
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(UiInspection::separate(*list, *buttons, dialog));
        for (auto *button : buttons->buttons())
            QVERIFY(UiInspection::fullyVisible(*button, dialog));
        QVERIFY(value(dialog, "Имя").size() > 1024);
        QVERIFY(list->topLevelItem(0)->text(1).endsWith(" ..."));
        QCOMPARE(list->topLevelItem(2)->text(1), QStringLiteral("Ошибка CRC 0x80000000"));
        QVERIFY(
            UiInspection::writeReport(dialog, QString("typed-properties-%1").arg(size.width())));
    }
};

QTEST_MAIN(PanelPropertyTests)
#include "panel_property_tests.moc"
