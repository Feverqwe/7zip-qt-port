#include "7zip/PropID.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "filemanager/PropertyName.h"
#include "localization/LangUtils.h"

#include <QTest>

static QString AsQString(const UString &s) { return QString::fromWCharArray(s); }

class PropertyNameTests : public QObject {
    Q_OBJECT

  private slots:
    void sourceResourceTakesPrecedenceOverCustomName() {
        QCOMPARE(AsQString(GetNameOfProperty(kpidName, L"custom")), QStringLiteral("Name"));
        QCOMPARE(AsQString(GetNameOfProperty(kpidSize, nullptr)), QStringLiteral("Size"));
        QCOMPARE(AsQString(GetNameOfProperty(kpidPackSize, nullptr)),
                 QStringLiteral("Packed Size"));
        QCOMPARE(AsQString(GetNameOfProperty(kpidCRC, L"wrong checksum caption")), QString("CRC"));
        QCOMPARE(AsQString(GetNameOfProperty(kpidOutName, nullptr)), QString("Out Name"));
        QCOMPARE(AsQString(GetNameOfProperty(kpidDevMinor, nullptr)), QString("Dev Minor"));
    }

    void missingResourceUsesCustomOrNumericFallback() {
        QCOMPARE(AsQString(GetNameOfProperty(999, L"свойство й 😀")),
                 QStringLiteral("свойство й 😀"));
        QCOMPARE(AsQString(GetNameOfProperty(999, nullptr)), QString("999"));
        QCOMPARE(AsQString(GetNameOfProperty(1000, L"external property")),
                 QString("external property"));
        QCOMPARE(AsQString(GetNameOfProperty(1000, nullptr)), QString("1000"));
        QCOMPARE(AsQString(GetNameOfProperty(0xFFFFFFFF, nullptr)), QString("4294967295"));
        QVERIFY(GetNameOfProperty(1000, L"").IsEmpty());
        QCOMPARE(AsQString(GetNameOfProperty(kpidNoProperty, nullptr)), QString("0"));
    }

    void resourceOverloadsReplaceOrAppendWithoutStaleText() {
        UString s(L"old value");
        LangString(IDS_PROP_PATH, s);
        QCOMPARE(AsQString(s), QStringLiteral("Path"));
        s += L": ";
        AddLangString(s, IDS_PROP_NAME);
        QCOMPARE(AsQString(s), QStringLiteral("Path: Name"));
        LangString(0xFFFFFFFF, s);
        QVERIFY(s.IsEmpty());
        QVERIFY(LangString(0xFFFFFFFF).IsEmpty());
        AddLangString(s, IDS_PROP_OUT_NAME);
        QCOMPARE(AsQString(s), QString("Out Name"));
    }
};

QTEST_GUILESS_MAIN(PropertyNameTests)
#include "property_name_tests.moc"
