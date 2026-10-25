#include "qtsmbmountplatform.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#ifdef Q_OS_MACOS
#include <csignal>
#include <unistd.h>
#endif

class NativeMountClientTests final : public QObject {
    Q_OBJECT
  private slots:
    void missingSystemClient() {
#ifdef Q_OS_LINUX
        const QString reason = QtSmbMountPlatform::unavailableReason();
        if (reason.isEmpty())
            QSKIP("System CIFS client is installed; missing-client scenario is unavailable.");
        QtSmbMountPlatform client;
        QSignalSpy result(&client, &QtSmbMountPlatform::mountFinished);
        client.mount(49152, "SyntheticShare", "szprobe", "SyntheticPassword", {});
        QCOMPARE(result.size(), 1);
        QVERIFY(result[0][0].toString().isEmpty());
        QCOMPARE(result[0][1].toString(), reason);
#else
        QSKIP("Linux system prerequisite scenario.");
#endif
    }
    void nativeLifecycle_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("explicit unmount") << 0;
        QTest::newRow("owner channel lost") << 1;
#ifdef Q_OS_MACOS
        QTest::newRow("broker killed then recovered") << 2;
#endif
    }
    void nativeLifecycle() {
        QFETCH(int, mode);
        if (qEnvironmentVariableIntValue("SEVENZIP_NATIVE_SMB_TEST") != 1)
            QSKIP("Opt-in native OS mount and administrator authorization.");
        const QString reason = QtSmbMountPlatform::unavailableReason();
        if (!reason.isEmpty())
            QSKIP(qPrintable(reason));
#ifdef Q_OS_UNIX
        QTemporaryDir target(QStringLiteral("/tmp/7nt-XXXXXX"));
#else
        QTemporaryDir target;
#endif
        QVERIFY(target.isValid());
        target.setAutoRemove(false);
        QProcess server;
        server.start(QStringLiteral(SEVENZIP_SMB_PROBE), {"0"});
        QVERIFY(server.waitForStarted(10000));
        QByteArray diagnostics;
        QRegularExpression ready("loopback SMB fixture ready on port ([0-9]+)");
        QRegularExpressionMatch match;
        QTRY_VERIFY_WITH_TIMEOUT((diagnostics += server.readAllStandardError(),
                                  match = ready.match(QString::fromUtf8(diagnostics)),
                                  match.hasMatch()),
                                 15000);
        auto client = std::make_unique<QtSmbMountPlatform>();
        QSignalSpy mounted(client.get(), &QtSmbMountPlatform::mountFinished);
        QSignalSpy unmounted(client.get(), &QtSmbMountPlatform::unmountFinished);
        client->mount(match.captured(1).toUShort(), "Sevenzip", "szprobe", "LocalProbeOnly427",
                      QFileInfo(target.path()).canonicalFilePath());
        QTRY_COMPARE_WITH_TIMEOUT(mounted.size(), 1, 60000);
        const QString path = mounted[0][0].toString();
        const QString error = mounted[0][1].toString();
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QVERIFY(!path.isEmpty());
        // Always ask the live client to detach before stopping its server, even
        // if a read assertion below fails.
        const auto cleanup = qScopeGuard([&] {
            if (client && (unmounted.isEmpty() || !unmounted.last()[0].toString().isEmpty())) {
                client->unmount(path, true);
                QTest::qWait(100);
                for (int attempt = 0; attempt < 300 && (unmounted.isEmpty() ||
                                                        !unmounted.last()[0].toString().isEmpty());
                     ++attempt) {
                    if (attempt && attempt % 10 == 0)
                        client->unmount(path, true);
                    QTest::qWait(100);
                }
            }
            server.terminate();
            if (!server.waitForFinished(5000)) {
                server.kill();
                server.waitForFinished(5000);
            }
            QDir().rmdir(target.path());
        });
        const QDir root(path);
        QCOMPARE(root.entryList(QDir::Files).size(), 3);
        QFile data(root.filePath(QStringLiteral("Файл с пробелом.bin")));
        QVERIFY(data.open(QIODevice::ReadOnly));
        QVERIFY(data.seek(100003));
        QByteArray expected(4096, Qt::Uninitialized);
        for (int i = 0; i < expected.size(); ++i)
            expected[i] = char((100003 + i) % 251);
        QCOMPARE(data.read(4096), expected);
        data.close();
        QFile forbidden(root.filePath("new.bin"));
        QVERIFY(!forbidden.open(QIODevice::WriteOnly));
        if (mode == 1) {
            client.reset();
            QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(root.filePath("hello.txt")), 45000);
        } else {
#ifdef Q_OS_MACOS
            if (mode == 2) {
                // Target only the broker spawned by this test process. Never
                // kill another application's mount helper or the SMB server.
                QProcess identify;
                identify.start("/usr/bin/pgrep",
                               {"-P", QString::number(QCoreApplication::applicationPid()), "-x",
                                "SevenzipMount"});
                QVERIFY(identify.waitForFinished(10000));
                const auto candidates = identify.readAllStandardOutput().trimmed().split('\n');
                QCOMPARE(candidates.size(), 1);
                bool valid = false;
                const auto broker = candidates.front().toLongLong(&valid);
                QVERIFY(valid && broker > 0);
                QCOMPARE(::kill(pid_t(broker), SIGKILL), 0);
                QTRY_COMPARE_WITH_TIMEOUT(unmounted.size(), 1, 10000);
                QVERIFY(!unmounted[0][0].toString().isEmpty());
                QFile stillReadable(root.filePath("hello.txt"));
                QVERIFY(stillReadable.open(QIODevice::ReadOnly));
                QCOMPARE(stillReadable.read(4), QByteArray::fromHex("00010203"));
                stillReadable.close();
                unmounted.clear();
            }
#endif
            client->unmount(path, false);
            QTRY_COMPARE_WITH_TIMEOUT(unmounted.size(), 1, 45000);
            QVERIFY2(unmounted[0][0].toString().isEmpty(), qPrintable(unmounted[0][0].toString()));
        }
    }
};
QTEST_GUILESS_MAIN(NativeMountClientTests)
#include "native_mount_client_tests.moc"
