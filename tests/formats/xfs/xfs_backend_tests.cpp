#include "7zip/Common/FileStreams.h"
#include "Common/StringConvert.h"
#include "archive/backend/archivebackend.h"
#include "archive/resources/archivedevice.h"
#include "archive/resources/archivenativeopen.h"
#include "devices/support/device_test_support.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class XfsBackendTests final : public QObject {
    Q_OBJECT
    QTemporaryDir temporary_;
    QString engine_, module_, image_;

  private slots:
    void initTestCase() {
        QVERIFY(temporary_.isValid());
        QProcess unpack;
        unpack.start(QString::fromUtf8(SEVENZIP_XFS_PYTHON),
                     {QString::fromUtf8(SEVENZIP_XFS_FIXTURES) + "/unpack.py",
                      QString::fromUtf8(SEVENZIP_XFS_FIXTURES), temporary_.path()});
        QVERIFY(unpack.waitForFinished(30000));
        QCOMPARE(unpack.exitCode(), 0);
        image_ = temporary_.filePath(QStringLiteral("сервер NAS 空.xfs"));
        QVERIFY(QFile::rename(temporary_.filePath("v5.xfs"), image_));
        const QDir prepared(QString::fromUtf8(SEVENZIP_XFS_RUNTIME));
#ifdef Q_OS_WIN
        const QString core = "7z.dll", client = "7z.exe", suffix = ".dll";
#else
        const QString core = "7z.so", client = "7z", suffix = ".so";
#endif
        engine_ = temporary_.filePath(client);
        module_ = temporary_.filePath(core);
        QVERIFY(QFile::copy(prepared.filePath(client), engine_));
        QVERIFY(QFile::copy(prepared.filePath(core), module_));
        QVERIFY(QDir(temporary_.path()).mkdir("Formats"));
        QVERIFY(QFile::copy(QString::fromUtf8(SEVENZIP_XFS_MODULE),
                            temporary_.filePath(QStringLiteral("Formats/формат XFS 空") + suffix)));
    }
    void listsTestsAndCopies() {
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        const auto catalog = backend.codecCatalog();
        QVERIFY(!catalog->canUpdate("XFS"));
        ArchiveInfo info;
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::infoReady, this,
                [&](const auto &value) { info = value; });
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const auto &value) { entries = value; });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(image_);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(info.format, QStringLiteral("XFS"));
        QCOMPARE(entries.size(), 607);
        finished.clear();
        backend.test(image_);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString filename = QStringLiteral("файл_空.bin");
        const QString output = temporary_.filePath("output");
        backend.extract(image_, output, {filename, QStringLiteral("nested/with-space.bin")});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QByteArray expected;
        for (int i = 0; i < 5000; ++i)
            expected += QStringLiteral("XFS: файл 空\n").toUtf8();
        expected += "tail\n";
        for (const auto &path : {filename, QStringLiteral("nested/with-space.bin")}) {
            QFile file(QDir(output).filePath(path));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), expected);
        }
        auto *file = new CInFileStream;
        CMyComPtr<IInStream> input = file, deviceInput;
        QVERIFY(file->Open(us2fs(image_.toStdWString().c_str())));
        QCOMPARE(DeviceInput(input, QFileInfo(image_).size(), &deviceInput, 4096), S_OK);
        COpenUi ui;
        QString error;
        std::shared_ptr<ArchiveNativeOpen> opened;
        QCOMPARE(OpenDeviceFilesystem(catalog, deviceInput, "NAS 空", &ui, opened, &error), S_OK);
        QVERIFY(opened);
        UInt32 count = 0;
        QCOMPARE(opened->arc().Archive->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 607u);
    }
};
QTEST_GUILESS_MAIN(XfsBackendTests)
#include "xfs_backend_tests.moc"
