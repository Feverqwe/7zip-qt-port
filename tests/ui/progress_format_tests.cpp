#include "ui/progress/ProgressDialog2.h"
#include <QtTest>
#include <algorithm>
#include <iterator>

class ProgressFormatTests : public QObject {
    Q_OBJECT

  private slots:
    void originalDurationFormatting_data() {
        QTest::addColumn<quint64>("seconds");
        QTest::addColumn<QString>("expected");
        QTest::newRow("zero") << quint64(0) << QString("00:00:00");
        QTest::newRow("seconds") << quint64(59) << QString("00:00:59");
        QTest::newRow("minute") << quint64(60) << QString("00:01:00");
        QTest::newRow("hour-boundary") << quint64(3599) << QString("00:59:59");
        QTest::newRow("hour") << quint64(3600) << QString("01:00:00");
        QTest::newRow("99-hours") << quint64(359999) << QString("99:59:59");
        QTest::newRow("100-hours") << quint64(360000) << QString("100:00:00");
        QTest::newRow("32-bit-maximum") << quint64(4294967295ULL) << QString("1193046:28:15");
        QTest::newRow("64-bit-maximum") << quint64(~UInt64(0)) << QString("5124095576030431:00:15");
    }
    void originalDurationFormatting() {
        QFETCH(quint64, seconds);
        QFETCH(QString, expected);
        wchar_t value[40];
        std::fill(std::begin(value), std::end(value), wchar_t(0x7fff));
        GetTimeString(seconds, value);
        QCOMPARE(QString::fromWCharArray(value), expected);
        QCOMPARE(value[expected.size()], wchar_t(0));
        QCOMPARE(value[expected.size() + 1], wchar_t(0x7fff));
    }
};

QTEST_GUILESS_MAIN(ProgressFormatTests)
#include "progress_format_tests.moc"
