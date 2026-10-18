#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include <QtTest>

class MemoryLimitTests final : public QObject {
    Q_OBJECT
  private slots:
    void parsing_data() {
        QTest::addColumn<QString>("spec");
        QTest::addColumn<bool>("valid");
        QTest::addColumn<bool>("percent");
        QTest::addColumn<quint64>("value");
        QTest::newRow("auto") << QString() << false << false << quint64(0);
        QTest::newRow("percent") << QString("80%") << true << true << quint64(80);
        QTest::newRow("alternate") << QString("P75") << true << true << quint64(75);
        QTest::newRow("zero") << QString("0%") << true << true << quint64(0);
        QTest::newRow("over100") << QString("110%") << true << true << quint64(110);
        QTest::newRow("bytes") << QString("123B") << true << false << quint64(123);
        QTest::newRow("no-unit") << QString("123") << true << false << quint64(123);
        QTest::newRow("kib") << QString("16kB") << true << false << quint64(16 << 10);
        QTest::newRow("mib") << QString("256M") << true << false << quint64(256 << 20);
        QTest::newRow("gib") << QString("2g") << true << false << (quint64(2) << 30);
        QTest::newRow("tib") << QString("3TB") << true << false << (quint64(3) << 40);
        QTest::newRow("maximum") << QString("18446744073709551615") << true << false << quint64(-1);
        for (const QString &s :
             {QString(" 80%"), QString("80% "), QString("1.5G"), QString("-1M"), QString("p80%"),
              QString("80%%"), QString("2GBx"), QString("16KiB"), QString("1P"),
              QString("18446744073709551616"), QString("16777216T"), QStringLiteral("память")})
            QTest::newRow(qPrintable(s)) << s << false << false << quint64(0);
    }
    void parsing() {
        QFETCH(QString, spec);
        QFETCH(bool, valid);
        QFETCH(bool, percent);
        QFETCH(quint64, value);
        NCompression::CMemUse mu;
        mu.Parse("90%");
        mu.Parse(spec);
        QCOMPARE(mu.IsDefined, valid);
        QCOMPARE(mu.IsPercent, percent);
        QCOMPARE(mu.Val, UInt64(value));
        QCOMPARE(mu.GetBytes(1000), percent ? UInt64(value * 10) : UInt64(value));
    }
    void percentOverflowAndRounding() {
        NCompression::CMemUse mu;
        mu.Parse("80%");
        QCOMPARE(mu.GetBytes(101), UInt64(80));
        mu.Parse("0%");
        QCOMPARE(mu.GetBytes(UInt64(-1)), UInt64(0));
        mu.Parse("100%");
        QCOMPARE(mu.GetBytes(UInt64(-1)), UInt64(-1));
        mu.Parse("200%");
        QCOMPARE(mu.GetBytes(UInt64(-1)), UInt64(-1));
        mu.Parse("99%");
        QCOMPARE(mu.GetBytes(UInt64(-1)), UInt64(-1) / 100 * 99);
    }
};
QTEST_GUILESS_MAIN(MemoryLimitTests)
#include "memory_limit_tests.moc"
