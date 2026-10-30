#include "archive/support/source_extract_fixture.h"
#include "ui/support/native_window_diagnostics.h"
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>
#include <windows.h>

namespace {
constexpr const wchar_t *kPrivateClassPath =
    L"Software\\Classes\\CLSID\\{52D06B47-DA74-4A5F-8B5E-4494C729CC9C}";
bool RemoveOwnedRegistration(DWORD pid) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kPrivateClassPath, 0, KEY_QUERY_VALUE, &key) !=
        ERROR_SUCCESS)
        return true;
    DWORD owner = 0, type = 0, size = sizeof(owner);
    const LONG queried = RegQueryValueExW(key, L"SevenzipTestHostPID", nullptr, &type,
                                          reinterpret_cast<BYTE *>(&owner), &size);
    RegCloseKey(key);
    if (queried != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(owner) || owner != pid)
        return true; // Never clean up another registration.
    return RegDeleteTreeW(HKEY_CURRENT_USER, kPrivateClassPath) == ERROR_SUCCESS;
}
struct OwnedWindow {
    DWORD process = 0;
    HWND window = nullptr;
    QString title;
};
BOOL CALLBACK FindOwnedWindow(HWND window, LPARAM parameter) {
    auto &owned = *reinterpret_cast<OwnedWindow *>(parameter);
    DWORD process = 0;
    GetWindowThreadProcessId(window, &process);
    if (process == owned.process && IsWindowVisible(window)) {
        wchar_t title[512]{};
        GetWindowTextW(window, title, 512);
        if (QString::fromWCharArray(title).contains(owned.title, Qt::CaseInsensitive)) {
            owned.window = window;
            return FALSE;
        }
    }
    return TRUE;
}
} // namespace

class NativeShellLaunchTests final : public QObject {
    Q_OBJECT
  private slots:
    void realDllExtractAndCancel_data() {
        QTest::addColumn<bool>("cancel");
        QTest::addColumn<bool>("broken");
        QTest::newRow("DLL-extract-here") << false << false;
        QTest::newRow("DLL-extract-dialog-cancel") << true << false;
        QTest::newRow("DLL-broken-archive-refusal") << false << true;
    }
    void realDllExtractAndCancel() {
        QFETCH(bool, cancel);
        QFETCH(bool, broken);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString staging = fixture.filePath("source");
        const QByteArray bytes("real DLL/map/helper extract bytes");
        const QString original = SourceExtractFixture::makeArchive(staging, bytes);
        QVERIFY(!original.isEmpty());
        const QByteArray originalBytes = SourceExtractFixture::readFile(original);
        const QString output = fixture.filePath(QStringLiteral("выход 空 with spaces"));
        QVERIFY(QDir().mkpath(output));
        const QString archive = QDir(output).filePath(QStringLiteral("архив 空 with spaces.zip"));
        QVERIFY(QFile::copy(original, archive));
        if (broken)
            QVERIFY(SourceExtractFixture::writeFile(archive, QByteArray("broken ZIP fixture")));
        const QByteArray archiveBytes = SourceExtractFixture::readFile(archive);
        const QString extracted = QDir(output).filePath(SourceExtractFixture::kName);
        const QString sentinel = QDir(output).filePath("keep.bin");
        QVERIFY(SourceExtractFixture::writeFile(sentinel, QByteArray("keep")));
        QProcess host;
        const QString installed = qEnvironmentVariable("NATIVE_SHELL_TEST_INSTALL_DIR");
        const QString library = installed.isEmpty() ? QString::fromUtf8(NATIVE_SHELL_LIBRARY)
                                                    : QDir(installed).filePath("bin/7-zip.dll");
        if (!installed.isEmpty()) {
            for (const QString &name : {"7zFM.exe", "7zG.exe", "7z.dll", "7-zip.dll"})
                QVERIFY2(QFileInfo::exists(QDir(installed).filePath("bin/" + name)),
                         qPrintable(name));
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            const QString system = environment.value("SystemRoot");
            QVERIFY(!system.isEmpty());
            const QString binary = QDir::toNativeSeparators(QDir(installed).filePath("bin"));
            environment.insert("PATH", binary + ";" + system + "\\System32;" + system);
            environment.insert("SEVENZIP_CODEC_MODULE_PATH", binary + "\\7z.dll");
            environment.insert("QT_PLUGIN_PATH", QString());
            environment.insert("QT_QPA_PLATFORM_PLUGIN_PATH", QString());
            host.setProcessEnvironment(environment);
        }
        QStringList arguments{QDir::toNativeSeparators(library), QDir::toNativeSeparators(archive),
                              cancel ? "SevenZipExtract" : "SevenZipExtractHere"};
        if (!installed.isEmpty() && qEnvironmentVariableIntValue("NATIVE_SHELL_TEST_REGISTER"))
            arguments << "--registered";
        host.start(QString::fromUtf8(NATIVE_SHELL_LAUNCH_HOST), arguments);
        QVERIFY(host.waitForStarted(5000));
        const DWORD hostPid = DWORD(host.processId());
        const auto stop = qScopeGuard([&] {
            if (host.state() != QProcess::NotRunning) {
                host.kill();
                host.waitForFinished(5000);
            }
            QVERIFY(RemoveOwnedRegistration(hostPid));
        });
        QByteArray report;
        QElapsedTimer reportWait;
        reportWait.start();
        while (!report.contains('\n') && host.state() != QProcess::NotRunning &&
               reportWait.elapsed() < 15000) {
            report += host.readAllStandardOutput();
            QTest::qWait(20);
        }
        const QByteArray diagnostics = report + host.readAllStandardError() +
                                       " host exit=" + QByteArray::number(host.exitCode()) + "\n" +
                                       NativeWindowDiagnostics(hostPid).join("\n").toUtf8();
        QVERIFY2(report.startsWith("helper="), diagnostics.constData());
        bool parsed = false;
        const DWORD pid = report.mid(7).trimmed().toUInt(&parsed);
        QVERIFY(parsed && pid != 0 && pid != DWORD(host.processId()));
        const HANDLE helper =
            OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        QVERIFY(helper);
        const auto release = qScopeGuard([&] { CloseHandle(helper); });
        OwnedWindow dialog{pid, nullptr, cancel ? QString("Extract") : QString()};
        if (!cancel && !broken) {
            QElapsedTimer extracting;
            extracting.start();
            while (SourceExtractFixture::readFile(extracted) != bytes &&
                   extracting.elapsed() < 15000 && WaitForSingleObject(helper, 0) != WAIT_OBJECT_0)
                QTest::qWait(20);
            const QByteArray windows = NativeWindowDiagnostics(pid).join("\n").toUtf8();
            QVERIFY2(SourceExtractFixture::readFile(extracted) == bytes, windows.constData());
        }
        if (broken) {
            QString failure;
            QElapsedTimer failed;
            failed.start();
            while (failed.elapsed() < 15000 && WaitForSingleObject(helper, 0) != WAIT_OBJECT_0) {
                failure = NativeWindowDiagnostics(pid).join("\n");
                if (failure.contains("Cannot open", Qt::CaseInsensitive) ||
                    failure.contains("Can not open", Qt::CaseInsensitive))
                    break;
                QTest::qWait(50);
            }
            QVERIFY2(WaitForSingleObject(helper, 0) == WAIT_OBJECT_0 ||
                         failure.contains("Cannot open", Qt::CaseInsensitive) ||
                         failure.contains("Can not open", Qt::CaseInsensitive),
                     qPrintable(failure));
        }
        QTRY_VERIFY_WITH_TIMEOUT(
            (EnumWindows(FindOwnedWindow, reinterpret_cast<LPARAM>(&dialog)),
             dialog.window || (!cancel && WaitForSingleObject(helper, 0) == WAIT_OBJECT_0)),
            20000);
        if (dialog.window)
            QVERIFY(PostMessageW(dialog.window, WM_CLOSE, 0, 0));
        if (broken) {
            // The source may retain its error list after the final error box.
            // Close only windows of this already-failed helper until it joins.
            QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                         if (WaitForSingleObject(helper, 0) == WAIT_OBJECT_0)
                                             return true;
                                         OwnedWindow remaining{pid, nullptr, QString()};
                                         EnumWindows(FindOwnedWindow,
                                                     reinterpret_cast<LPARAM>(&remaining));
                                         if (remaining.window)
                                             PostMessageW(remaining.window, WM_CLOSE, 0, 0);
                                         return false;
                                     }()),
                                     10000);
        }
        QTRY_COMPARE_WITH_TIMEOUT(WaitForSingleObject(helper, 0), DWORD(WAIT_OBJECT_0), 10000);
        DWORD exitCode = 0;
        QVERIFY(GetExitCodeProcess(helper, &exitCode));
        QCOMPARE(exitCode, DWORD(cancel ? 255 : broken ? 2 : 0));
        host.write("\n");
        QVERIFY(host.waitForFinished(5000));
        QCOMPARE(host.exitStatus(), QProcess::NormalExit);
        QCOMPARE(host.exitCode(), 0);
        if (!installed.isEmpty() && qEnvironmentVariableIntValue("NATIVE_SHELL_TEST_REGISTER")) {
            HKEY key = nullptr;
            const LONG present =
                RegOpenKeyExW(HKEY_CURRENT_USER, kPrivateClassPath, 0, KEY_QUERY_VALUE, &key);
            if (key)
                RegCloseKey(key);
            QCOMPARE(present, LONG(ERROR_FILE_NOT_FOUND));
        }
        if (cancel || broken)
            QVERIFY(!QFileInfo::exists(extracted));
        QCOMPARE(SourceExtractFixture::readFile(sentinel), QByteArray("keep"));
        QCOMPARE(SourceExtractFixture::readFile(archive), archiveBytes);
        QCOMPARE(SourceExtractFixture::readFile(original), originalBytes);
        QCOMPARE(
            SourceExtractFixture::readFile(QDir(staging).filePath(SourceExtractFixture::kName)),
            bytes);
    }
};

QTEST_GUILESS_MAIN(NativeShellLaunchTests)
#include "native_shell_launch_tests.moc"
