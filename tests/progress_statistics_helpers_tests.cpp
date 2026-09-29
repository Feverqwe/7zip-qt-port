// Compile the authoritative module in this test translation unit to retain
// upstream file-local helper visibility; do not add a production test API.
#include "CPP/7zip/UI/FileManager/ProgressDialog2.cpp"
#include <QtTest>
#include <algorithm>
#include <iterator>

class ProgressStatisticsHelpersTests : public QObject {
    Q_OBJECT

  private slots:
    void sourceSizeThresholds_data() {
        QTest::addColumn<quint64>("bytes");
        QTest::addColumn<QString>("expected");
        QTest::newRow("zero") << quint64(0) << QString("0");
        QTest::newRow("last-byte-unit") << quint64(99999) << QString("99999");
        QTest::newRow("first-KB") << quint64(100000) << QString("97 KB");
        QTest::newRow("last-KB") << quint64((100000ULL << 10) - 1) << QString("99999 KB");
        QTest::newRow("first-MB") << quint64(100000ULL << 10) << QString("97 MB");
        QTest::newRow("last-MB") << quint64((100000ULL << 20) - 1) << QString("99999 MB");
        QTest::newRow("first-GB") << quint64(100000ULL << 20) << QString("97 GB");
        QTest::newRow("maximum") << quint64(~UInt64(0)) << QString("17179869183 GB");
    }
    void sourceSizeThresholds() {
        QFETCH(quint64, bytes);
        QFETCH(QString, expected);
        wchar_t value[40];
        std::fill(std::begin(value), std::end(value), wchar_t(0x7fff));
        ConvertSizeToString(bytes, value);
        QCOMPARE(QString::fromWCharArray(value), expected);
        QCOMPARE(value[expected.size()], wchar_t(0));
        QCOMPARE(value[expected.size() + 1], wchar_t(0x7fff));
    }
    void sourceOverflowReduction() {
        QCOMPARE(MyMultAndDiv(12000, 750, 250), UInt64(36000));
        QCOMPARE(MyMultAndDiv(0, ~UInt64(0), 10), UInt64(0));
        QCOMPARE(MyMultAndDiv(12, 3, 0), UInt64(36));
        // The source also shifts the divisor to zero for this boundary.
        QCOMPARE(MyMultAndDiv(UInt64(1) << 63, 2, 2), UInt64(1) << 62);
        QCOMPARE(MyMultAndDiv(UInt64(1) << 32, UInt64(1) << 32, UInt64(1) << 32), UInt64(1) << 32);
        // Original overflow handling discards low bits; keep that approximation.
        QCOMPARE(MyMultAndDiv(~UInt64(0), ~UInt64(0), ~UInt64(0)), UInt64(18446744065119617025ULL));
        QCOMPARE(MyMultAndDiv(UInt64(1) << 63, 2, 1), UInt64(1) << 62);
    }
    void nativeStringReductionAndChangeDetection() {
        UString s(L"путь with spaces файл");
        ReduceString(s, 8);
        QCOMPARE(QString::fromWCharArray(s), QStringLiteral("путь ... файл"));
        ReduceString(s, 0);
        QCOMPARE(QString::fromWCharArray(s), QStringLiteral(" ... "));
        UString unchanged(L"файл");
        ReduceString(unchanged, 4);
        QCOMPARE(QString::fromWCharArray(unchanged), QStringLiteral("файл"));
        UString previous;
        bool changed = false;
        GetChangedString(unchanged, previous, changed);
        QVERIFY(changed);
        QCOMPARE(QString::fromWCharArray(previous), QStringLiteral("файл"));
        changed = true;
        GetChangedString(unchanged, previous, changed);
        QVERIFY(!changed);
        GetChangedString(UString(), previous, changed);
        QVERIFY(changed);
        QVERIFY(previous.IsEmpty());
    }
    void nativePowerAndMessageDigitBoundaries() {
        QCOMPARE(GetPower32(0), unsigned(0));
        QCOMPARE(GetPower64(0), unsigned(0));
        for (unsigned bit = 0; bit < 64; ++bit) {
            const UInt64 value = UInt64(1) << bit;
            QCOMPARE(GetPower64(value), bit + 1);
            QCOMPARE(GetPower64(value - 1), bit);
            if (bit < 32) {
                QCOMPARE(GetPower32(UInt32(value)), bit + 1);
                QCOMPARE(GetPower32(UInt32(value - 1)), bit);
            }
        }
        QCOMPARE(GetPower64(~UInt64(0)), unsigned(64));
        QCOMPARE(GetPower32(~UInt32(0)), unsigned(32));
        QCOMPARE(GetNumDigits(0), unsigned(1));
        QCOMPARE(GetNumDigits(9), unsigned(1));
        QCOMPARE(GetNumDigits(10), unsigned(2));
        QCOMPARE(GetNumDigits(99), unsigned(2));
        QCOMPARE(GetNumDigits(100), unsigned(3));
        QCOMPARE(GetNumDigits(999999), unsigned(6));
        QCOMPARE(GetNumDigits(1000000), unsigned(7));
        QCOMPARE(GetNumDigits(~UInt32(0)), unsigned(10));
    }
};

QTEST_GUILESS_MAIN(ProgressStatisticsHelpersTests)
#include "progress_statistics_helpers_tests.moc"
