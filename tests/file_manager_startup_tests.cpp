#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "archivecodeccatalog.h"
#include "mainwindow.h"
#include "qtfilemanagerstartup.h"
#include "source_extract_fixture.h"
#include <QScopeGuard>
#include <QSignalSpy>
#include <QtTest>
#ifdef Q_OS_WIN
#include <windows.h>

namespace {
struct NativeStartupWindow {
    DWORD PID;
    QString Title;
    bool Refusal;
    HWND Window = nullptr;
};
BOOL CALLBACK FindStartupWindow(HWND window, LPARAM parameter) {
    auto &request = *reinterpret_cast<NativeStartupWindow *>(parameter);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid != request.PID || !IsWindowVisible(window))
        return TRUE;
    wchar_t title[2048]{};
    GetWindowTextW(window, title, 2048);
    const QString text = QString::fromWCharArray(title);
    const LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
    const bool matches = request.Refusal ? text == request.Title && (style & WS_POPUP)
                                         : text.contains(request.Title);
    if (!matches)
        return TRUE;
    request.Window = window;
    return FALSE;
}
} // namespace
#endif

class FileManagerStartupTests final : public QObject {
    Q_OBJECT
    QString module, engine;
    quint32 version = 0;

  private slots:
    void initTestCase() {
        module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        engine = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine.isEmpty());
        ArchiveCodecCatalog probe(module, 0);
        QCOMPARE(probe.result(), S_OK);
        version = probe.version();
    }
    void initialNativeBinding_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("tail");
        QTest::addColumn<bool>("refused");
        QTest::newRow("quoted-unicode-auto") << QString("zip") << QString() << false;
        QTest::newRow("quoted-unicode-forced-zip") << QString("zip") << QString(" -tzip") << false;
        QTest::newRow("wrong-forced-type") << QString("zip") << QString(" -t7z") << true;
        QTest::newRow("broken-archive") << QString("broken") << QString(" -tzip") << true;
        QTest::newRow("encrypted-open-cancel") << QString("encrypted") << QString(" -t7z") << true;
        QTest::newRow("relative-folder") << QString("folder") << QString() << false;
        QTest::newRow("source-ignored-tail")
            << QString("zip") << QString(" -ignored -t7z") << false;
    }
    void initialNativeBinding() {
        QFETCH(QString, kind);
        QFETCH(QString, tail);
        QFETCH(bool, refused);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString cwd = QDir::currentPath();
        const auto restore = qScopeGuard([&] { QDir::setCurrent(cwd); });
        QVERIFY(QDir::setCurrent(fixture.path()));
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        const QString name = QStringLiteral("payload 密碼 with spaces.txt");
        const QByteArray bytes("native File Manager startup bytes");
        QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), bytes));
        const bool encrypted = kind == "encrypted";
        QString path = fixture.filePath(encrypted ? QStringLiteral("архив 空 with spaces.7z")
                                                  : QStringLiteral("архив 空 with spaces.zip"));
        if (kind == "folder") {
            path = QStringLiteral("каталог 空 with spaces");
            QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(path + "/" + name), bytes));
        } else if (kind == "broken") {
            QVERIFY(SourceExtractFixture::writeFile(path, "invalid ZIP fixture"));
        } else {
            QProcess create;
            QStringList arguments{"a", encrypted ? "-t7z" : "-tzip"};
            if (encrypted)
                arguments << "-pfixture" << "-mhe=on";
            arguments << path << name;
            create.start(engine, arguments);
            QVERIFY(create.waitForFinished(10000));
            QCOMPARE(create.exitCode(), 0);
        }
        const QByteArray original =
            kind == "folder" ? QByteArray() : SourceExtractFixture::readFile(path);
        const QString line = QStringLiteral("\"C:\\Sevenzip Desktop\\7zFM.exe\" \"") + path +
                             QStringLiteral("\"") + tail;
        const auto startup = WinMain2(ToUString(line));
        QCOMPARE(startup.NeedOpenArc, kind != "folder");
        QCOMPARE(QDir::fromNativeSeparators(ToQString(startup.FullPath)),
                 QFileInfo(path).absoluteFilePath());
        ArchiveBackend backend(nullptr, engine);
        QCOMPARE(backend.loadCodecCatalog(module, version), S_OK);
        int passwordRequests = 0;
        QTimer cancellation;
        connect(&cancellation, &QTimer::timeout, [&] {
            if (auto *password =
                    dynamic_cast<CPasswordDialog *>(QApplication::activeModalWidget())) {
                ++passwordRequests;
                password->reject();
            }
        });
        if (encrypted)
            cancellation.start(5);
        bool rejected = false;
        try {
            MainWindow window(&backend, {}, nullptr, &startup);
            window.show();
            auto *table = window.findChild<QTableWidget *>("archiveTable");
            QVERIFY(table);
            QTRY_VERIFY(table->rowCount() > 0);
            bool found = false;
            for (int row = 0; row < table->rowCount(); ++row)
                found |= table->item(row, 0)->text() == name;
            QVERIFY(found);
        } catch (const QtFileManagerStartupFailure &failure) {
            rejected = true;
            QCOMPARE(failure.Message.IsEmpty(), encrypted);
        }
        cancellation.stop();
        QCOMPARE(rejected, refused);
        QCOMPARE(passwordRequests > 0, encrypted);
        QVERIFY(!backend.busy());
        QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(name)), bytes);
        if (kind != "folder")
            QCOMPARE(SourceExtractFixture::readFile(path), original);
    }
#ifdef Q_OS_WIN
    void actualFileManagerProcess_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<bool>("refused");
        QTest::newRow("actual-7zFM-forced-zip") << QString("zip") << false;
        QTest::newRow("actual-7zFM-wrong-type") << QString("7z") << true;
    }
    void actualFileManagerProcess() {
        QFETCH(QString, type);
        QFETCH(bool, refused);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QByteArray bytes("actual native 7zFM process startup");
        const QString original = SourceExtractFixture::makeArchive(fixture.path(), bytes);
        QVERIFY(!original.isEmpty());
        const QString archive = fixture.filePath(QStringLiteral("архив 空 with spaces.zip"));
        QVERIFY(QFile::rename(original, archive));
        const QByteArray archiveBytes = SourceExtractFixture::readFile(archive);
        QProcess process;
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("SEVENZIP_SETTINGS_PATH", fixture.filePath("fm-settings.ini"));
        environment.remove("QT_QPA_PLATFORM");
        const QString installed = qEnvironmentVariable("NATIVE_SHELL_TEST_INSTALL_DIR");
        QString executable = QString(SEVENZIP_NATIVE_FM);
        if (!installed.isEmpty()) {
            const QString system = environment.value("SystemRoot");
            QVERIFY(!system.isEmpty());
            const QString binary = QDir::toNativeSeparators(QDir(installed).filePath("bin"));
            executable = binary + "\\7zFM.exe";
            environment.insert("PATH", binary + ";" + system + "\\System32;" + system);
            environment.insert("SEVENZIP_ENGINE_PATH", binary + "\\7z.exe");
            environment.insert("QT_PLUGIN_PATH", QString());
            environment.insert("QT_QPA_PLATFORM_PLUGIN_PATH", QString());
        }
        process.setProcessEnvironment(environment);
        process.setWorkingDirectory(fixture.path());
        const auto stop = qScopeGuard([&] {
            if (process.state() != QProcess::NotRunning) {
                process.kill();
                process.waitForFinished(5000);
            }
        });
        process.start(executable, {archive, "-t" + type});
        QVERIFY(process.waitForStarted(5000));
        NativeStartupWindow window{DWORD(process.processId()),
                                   refused ? QString("7-Zip") : QFileInfo(archive).fileName(),
                                   refused};
        QElapsedTimer timeout;
        timeout.start();
        while (!window.Window && timeout.elapsed() < 15000 &&
               process.state() != QProcess::NotRunning) {
            EnumWindows(FindStartupWindow, reinterpret_cast<LPARAM>(&window));
            QTest::qWait(20);
        }
        QVERIFY2(window.Window, process.readAll().constData());
        QVERIFY(PostMessageW(window.Window, WM_CLOSE, 0, 0));
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(SourceExtractFixture::readFile(archive), archiveBytes);
        QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(SourceExtractFixture::kName)),
                 bytes);
    }
#endif
};

QTEST_MAIN(FileManagerStartupTests)
#include "file_manager_startup_tests.moc"
