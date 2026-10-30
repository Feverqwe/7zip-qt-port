#include "Common/StringConvert.h"
#include "Windows/FileIO.h"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

namespace {
FString native(const QString &path) { return us2fs(UString(path.toStdWString().c_str())); }
QByteArray read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
CFiTime oldModificationTime() {
    CFiTime result{};
#ifdef _WIN32
    const UInt64 ticks = 125911584000000000ULL; // 2000-01-01 UTC.
    result.dwHighDateTime = DWORD(ticks >> 32);
    result.dwLowDateTime = DWORD(ticks);
#else
    result.tv_sec = 946684800;
#endif
    return result;
}
} // namespace

class FileOutputLifecycleTests final : public QObject {
    Q_OBJECT
  private slots:
    void reopeningKeepsMetadataWithItsFile_data() {
        QTest::addColumn<bool>("explicitClose");
        QTest::addColumn<bool>("refuseNext");
        QTest::newRow("implicit-close-next-file") << false << false;
        QTest::newRow("implicit-close-rejected-open") << false << true;
        QTest::newRow("explicit-close-next-file") << true << false;
    }
    void reopeningKeepsMetadataWithItsFile() {
        QFETCH(bool, explicitClose);
        QFETCH(bool, refuseNext);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString first = fixture.filePath(QStringLiteral("первый 空 with spaces.bin"));
        const QString second = fixture.filePath(refuseNext ? "missing/second.bin" : "second.bin");
        NWindows::NFile::NIO::COutFile output;
        QVERIFY(output.Create_NEW(native(first)));
        QVERIFY(output.WriteFull("first", 5));
        const CFiTime modified = oldModificationTime();
        QVERIFY(output.SetMTime(&modified));
        if (explicitClose)
            QVERIFY(output.Close());
        QCOMPARE(output.Create_NEW(native(second)), !refuseNext);
        if (!refuseNext)
            QVERIFY(output.WriteFull("second", 6));
        QVERIFY(output.Close());
        QCOMPARE(read(first), QByteArray("first"));
        QCOMPARE(QFileInfo(first).lastModified().toSecsSinceEpoch(), qint64(946684800));
        if (!refuseNext) {
            QCOMPARE(read(second), QByteArray("second"));
            QVERIFY(QFileInfo(second).lastModified().toSecsSinceEpoch() != qint64(946684800));
        }
    }
};

QTEST_GUILESS_MAIN(FileOutputLifecycleTests)
#include "file_output_lifecycle_tests.moc"
