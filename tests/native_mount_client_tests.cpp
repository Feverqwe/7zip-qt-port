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
#elif defined(Q_OS_WIN)
// clang-format off
#include <windows.h>
#include <tlhelp32.h>
// clang-format on
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
        QTest::newRow("case-distinct files") << 3;
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
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
        QStringList arguments{"0"};
        if (mode == 3)
            arguments.append("--case-names");
        server.start(QStringLiteral(SEVENZIP_SMB_PROBE), arguments);
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
            expected[i] = char((100003 + i + (mode == 3 ? 3 : 0)) % 251);
        QCOMPARE(data.read(4096), expected);
        data.close();
        if (mode == 3) {
            const QDir directory(root.filePath("xtables"));
            const auto names = directory.entryList(QDir::Files);
            QVERIFY(names.contains("libip6t_hl.so"));
            QVERIFY(names.contains("libip6t_HL.so"));
            for (int repeat = 0; repeat < 3; ++repeat) {
                for (const auto &name : {"libip6t_hl.so", "libip6t_HL.so"}) {
                    QFile entry(directory.filePath(name));
                    QVERIFY(entry.open(QIODevice::ReadOnly));
                    QCOMPARE(entry.read(1),
                             QByteArray(1, name == QByteArray("libip6t_hl.so") ? 6 : 7));
                }
            }
        }
        QFile forbidden(root.filePath("new.bin"));
        QVERIFY(!forbidden.open(QIODevice::WriteOnly));
        if (mode == 1) {
            client.reset();
            QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(root.filePath("hello.txt")), 45000);
        } else {
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
            if (mode == 2) {
                // Target only the broker spawned by this test process. Never
                // kill another application's mount helper or the SMB server.
#ifdef Q_OS_MACOS
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
#else
                const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                QVERIFY(snapshot != INVALID_HANDLE_VALUE);
                const auto closeSnapshot = qScopeGuard([&] { CloseHandle(snapshot); });
                PROCESSENTRY32W entry{};
                entry.dwSize = sizeof(entry);
                QList<DWORD> candidates;
                if (Process32FirstW(snapshot, &entry)) {
                    do {
                        if (entry.th32ParentProcessID == GetCurrentProcessId() &&
                            QString::fromWCharArray(entry.szExeFile)
                                    .compare(QStringLiteral("SevenzipMount.exe"),
                                             Qt::CaseInsensitive) == 0)
                            candidates.append(entry.th32ProcessID);
                    } while (Process32NextW(snapshot, &entry));
                }
                QCOMPARE(candidates.size(), 1);
                const HANDLE broker = OpenProcess(PROCESS_TERMINATE, FALSE, candidates.front());
                QVERIFY(broker);
                const auto closeBroker = qScopeGuard([&] { CloseHandle(broker); });
                QVERIFY(TerminateProcess(broker, 1));
#endif
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
