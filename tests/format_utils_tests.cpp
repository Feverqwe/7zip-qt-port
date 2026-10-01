#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include <QtTest>

class FormatUtilsTests : public QObject {
    Q_OBJECT

    static QString text(const UString &value) { return QString::fromWCharArray(value); }

  private slots:
    void originalUnsignedConversion_data() {
        QTest::addColumn<quint64>("number");
        QTest::addColumn<QString>("expected");
        QTest::newRow("zero") << quint64(0) << QString("0");
        QTest::newRow("32-bit-boundary") << quint64(4294967296ULL) << QString("4294967296");
        QTest::newRow("maximum") << quint64(~UInt64(0)) << QString("18446744073709551615");
    }
    void originalUnsignedConversion() {
        QFETCH(quint64, number);
        QFETCH(QString, expected);
        QCOMPARE(text(NumberToString(number)), expected);
    }

    void originalPlaceholderReplacement_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("argument");
        QTest::addColumn<QString>("expected");
        QTest::newRow("unicode-space") << QString("File '{0}'") << QStringLiteral("архив й.7z")
                                       << QStringLiteral("File 'архив й.7z'");
        QTest::newRow("multiple") << QString("{0}/{0}\n{1} %1") << QStringLiteral("имя")
                                  << QStringLiteral("имя/имя\n{1} %1");
        QTest::newRow("empty-argument") << QString("a{0}{0}b") << QString() << QString("ab");
        QTest::newRow("no-placeholder")
            << QString("{1} literal") << QString("unused") << QString("{1} literal");
        QTest::newRow("literal-placeholder-argument")
            << QString("{0}{0}") << QString("{0}") << QString("{0}{0}");
        QTest::newRow("empty-format") << QString() << QString("unused") << QString();
    }
    void originalPlaceholderReplacement() {
        QFETCH(QString, format);
        QFETCH(QString, argument);
        QFETCH(QString, expected);
        const UString nativeFormat(format.toStdWString().c_str());
        const UString nativeArgument(argument.toStdWString().c_str());
        QCOMPARE(text(MyFormatNew(nativeFormat, nativeArgument)), expected);
        QCOMPARE(text(nativeFormat), format);
        QCOMPARE(text(nativeArgument), argument);
    }

    void sourceResourceLookupFormatsUpdateCaptions() {
        const UString name = L"папка с пробелом/архив й.7z";
        QCOMPARE(text(MyFormatNew(IDS_WANT_UPDATE_MODIFIED_FILE, name)),
                 QStringLiteral("File 'папка с пробелом/архив й.7z' was modified.\nDo you want to "
                                "update it in the archive?"));
        QCOMPARE(text(MyFormatNew(IDS_CANNOT_UPDATE_FILE, name)),
                 QStringLiteral("Cannot update file\n'папка с пробелом/архив й.7z'"));
        QCOMPARE(text(MyFormatNew(IDS_BOOKMARK, name)), QString("Bookmark"));
        QCOMPARE(text(MyFormatNew(UINT(0xffff), name)), QString());
    }
};

QTEST_GUILESS_MAIN(FormatUtilsTests)
#include "format_utils_tests.moc"
