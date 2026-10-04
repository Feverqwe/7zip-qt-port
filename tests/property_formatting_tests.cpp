#include "7zip/PropID.h"
#include "7zip/UI/Common/PropIDUtils.h"
#include "Common/StringConvert.h"
#include "Windows/FileIO.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include <QDataStream>
#include <QFile>
#include <QScopedValueRollback>
#include <QtTest>
#include <limits>

using NWindows::NCOM::CPropVariant;

class PropertyFormattingTests : public QObject {
    Q_OBJECT

    static QString text(const PROPVARIANT &prop, PROPID id, int level = 0) {
        UString result;
        ConvertPropertyToString2(result, prop, id, level);
        return QString::fromWCharArray(result, result.Len());
    }
    static QByteArray linkData(const QString &target) {
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        const quint16 nameBytes = quint16(target.size() * 2);
        out << quint32(Z7_WIN_IO_REPARSE_TAG_SYMLINK) << quint16(12 + nameBytes * 2) << quint16(0)
            << quint16(0) << nameBytes << nameBytes << nameBytes
            << quint32(Z7_WIN_SYMLINK_FLAG_RELATIVE);
        for (int copy = 0; copy < 2; ++copy)
            for (QChar c : target)
                out << quint16(c.unicode());
        return data;
    }

  private slots:
#ifndef Q_OS_WIN
    void nativeLinkReadAndFailedReload() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString target = temp.filePath(QStringLiteral("файл й с пробелом.txt"));
        QFile file(target);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), qint64(7));
        file.close();
        const FString link(QFile::encodeName(temp.filePath("link")).constData());
        const auto wideTarget = target.toStdWString();
        QVERIFY(NWindows::NFile::NIO::SetSymLink_UString(link, UString(wideTarget.c_str())));
        CByteBuffer data;
        QVERIFY(NWindows::NFile::NIO::GetReparseData(link, data));
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(data.ConstData()), int(data.Size())),
                 target.toUtf8());
        QVERIFY(!NWindows::NFile::NIO::SetSymLink_UString(link, UString(wideTarget.c_str())));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("fixture"));
        file.close();
        QVERIFY(QFile::remove(temp.filePath("link")));
        QVERIFY(!NWindows::NFile::NIO::GetReparseData(link, data));
        QCOMPARE(data.Size(), size_t(0));
    }
#endif
    void typedValuesAndSpecialProperties() {
        CPropVariant prop;
        QCOMPARE(text(prop, kpidSize), QString());
        prop = UInt64(std::numeric_limits<UInt64>::max());
        QCOMPARE(text(prop, kpidSize), QStringLiteral("18446744073709551615"));
        prop = UInt32(0x00deadbe);
        QCOMPARE(text(prop, kpidCRC), QStringLiteral("00DEADBE"));
        prop = true;
        QCOMPARE(text(prop, kpidIsDir), QStringLiteral("+"));
        prop = false;
        QCOMPARE(text(prop, kpidIsDir), QStringLiteral("-"));
        prop = L"Unicode й 😀 with spaces";
        QCOMPARE(text(prop, kpidName), QStringLiteral("Unicode й 😀 with spaces"));
        prop.Clear();
        prop.vt = VT_I4;
        prop.lVal = -42;
        QCOMPARE(text(prop, kpidSize), QStringLiteral("-42"));
        prop = UInt32(0100644);
        QCOMPARE(text(prop, kpidPosixAttrib), QStringLiteral("-rw-r--r--"));
        prop = UInt32(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY);
        QCOMPARE(text(prop, kpidAttrib), QStringLiteral("RD"));
        prop = UInt64((UInt64(3) << 48) | 27);
        QCOMPARE(text(prop, kpidINode), QStringLiteral("3-27"));
        prop = UInt32(0xab);
        QCOMPARE(text(prop, kpidVa), QStringLiteral("0xAB"));
    }

    void timestampPrecisionAndEmptyTime() {
        QScopedValueRollback<bool> utc(g_Timestamp_Show_UTC, true);
        CPropVariant prop;
        prop.vt = VT_FILETIME;
        const UInt64 ticks = UInt64(11644473600ULL) * 10000000 + 1234567;
        prop.filetime.dwLowDateTime = UInt32(ticks);
        prop.filetime.dwHighDateTime = UInt32(ticks >> 32);
        prop.wReserved1 = k_PropVar_TimePrec_1ns;
        prop.wReserved2 = 89;
        QCOMPARE(text(prop, kpidMTime, 9), QStringLiteral("1970-01-01 00:00:00.123456789Z"));
        QCOMPARE(text(prop, kpidMTime, 0), QStringLiteral("1970-01-01 00:00:00Z"));
        prop.wReserved1 = k_PropVar_TimePrec_Unix;
        QCOMPARE(text(prop, kpidMTime, 9), QStringLiteral("1970-01-01 00:00:00Z"));
        prop.filetime = {};
        prop.wReserved2 = 0;
        QVERIFY(text(prop, kpidMTime, 9).isEmpty());
    }

    void reparseTargetAndTruncatedData() {
        const QString target = QStringLiteral("папка й/файл 😀.txt");
        QByteArray data = linkData(target);
        UString result;
        QVERIFY(ConvertNtReparseToString(reinterpret_cast<const Byte *>(data.constData()),
                                         data.size(), result));
        QCOMPARE(QString::fromWCharArray(result), target);
        for (int size = 0; size < data.size(); ++size) {
            const QByteArray truncated(data.constData(), size);
            QVERIFY(!ConvertNtReparseToString(reinterpret_cast<const Byte *>(truncated.constData()),
                                              truncated.size(), result));
            QVERIFY(result.IsEmpty());
        }
        data[6] = 1; // A reserved field is a minor source warning, not a read failure.
        QVERIFY(ConvertNtReparseToString(reinterpret_cast<const Byte *>(data.constData()),
                                         data.size(), result));
        QCOMPARE(QString::fromWCharArray(result), target + " : MINOR_ERROR");
        data[6] = 0;
        data[8] = char(0xff); // Invalid substitute offset must not become a target path.
        NWindows::NFile::CReparseAttr attr;
        QVERIFY(!attr.Parse(reinterpret_cast<const Byte *>(data.constData()), data.size()));
        QVERIFY(ConvertNtReparseToString(reinterpret_cast<const Byte *>(data.constData()),
                                         data.size(), result));
        QVERIFY(QString::fromWCharArray(result).startsWith("SYMLINK:"));
    }

    void unsupportedReparseTagAndInvalidSecurity() {
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out << quint32(0x87654321) << quint16(0) << quint16(0);
        UString result;
        QVERIFY(ConvertNtReparseToString(reinterpret_cast<const Byte *>(data.constData()),
                                         data.size(), result));
        QCOMPARE(QString::fromWCharArray(result), QStringLiteral("REPARSE:87654321:0"));
        AString security;
        ConvertNtSecureToString(reinterpret_cast<const Byte *>(data.constData()), data.size(),
                                security);
        QCOMPARE(QByteArray(security), QByteArray("ERROR"));
        data = QByteArray(20, 0);
        data[0] = 2;
        ConvertNtSecureToString(reinterpret_cast<const Byte *>(data.constData()), data.size(),
                                security);
        QCOMPARE(QByteArray(security), QByteArray("UNSUPPORTED"));
#ifdef _WIN32
        QVERIFY(!CheckNtSecure(reinterpret_cast<const Byte *>(data.constData()), 8));
#endif
    }
};

QTEST_GUILESS_MAIN(PropertyFormattingTests)
#include "property_formatting_tests.moc"
