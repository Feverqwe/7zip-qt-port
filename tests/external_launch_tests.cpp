#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "qtarchivefoldermanager.h"
#include "qtassociatedfileservices.h"
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

namespace {
class CAssociationService final : public IQtExternalFileServices {
  public:
    QString Opened;
    HRESULT Result = S_OK;
    HRESULT StartApplication(const UString &, const UString &path, QWidget *) override {
        Opened = ToQString(path);
        return Result; // Fake association with no handle; never starts a user application.
    }
};
bool write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
} // namespace

class ExternalLaunchTests final : public QObject {
    Q_OBJECT
  private slots:
    void associationAdapterPreservesPathAndReportsFailure() {
        const UString path(L"/папка с пробелом/имя <b>файла</b>.txt");
        QUrl seen;
        QtAssociatedFileServices success([&](const QUrl &url) {
            seen = url;
            return true;
        });
        QCOMPARE(success.StartApplication(UString(), path, nullptr), S_OK);
        QVERIFY(seen.isLocalFile());
        QCOMPARE(seen.toLocalFile(), ToQString(path));
        bool plain = false;
        QTimer::singleShot(0, qApp, [&] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (box) {
                plain = box->textFormat() == Qt::PlainText && box->text().contains(ToQString(path));
                box->accept();
            }
        });
        QtAssociatedFileServices failure([](const QUrl &) -> bool { throw 1; });
        QCOMPARE(failure.StartApplication(UString(), path, nullptr), E_FAIL);
        QVERIFY(plain);
    }
    void associationWithoutHandleRetainsFileUntilOwnerExit_data() {
        QTest::addColumn<bool>("changed");
        QTest::addColumn<bool>("launchFailed");
        QTest::addColumn<bool>("tryInternal");
        QTest::newRow("unchanged-cleanup") << false << false << false;
        QTest::newRow("changed-preserve") << true << false << false;
        QTest::newRow("launch-failure-cleanup") << false << true << false;
        QTest::newRow("memory-probe-fallback") << false << false << true;
    }
    void associationWithoutHandleRetainsFileUntilOwnerExit() {
        QFETCH(bool, changed);
        QFETCH(bool, launchFailed);
        QFETCH(bool, tryInternal);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("файл с пробелом.bin");
        QVERIFY(write(fixture.filePath(name), "contents"));
        QProcess process;
        process.setWorkingDirectory(fixture.path());
        process.start(QStandardPaths::findExecutable("7zz"), {"a", "-tzip", "original.zip", name});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        const QString archive = fixture.filePath("original.zip");
        ArchiveBackend backend;
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CAssociationService service;
        service.Result = launchFailed ? E_FAIL : S_OK;
        {
            CPanel panel;
            panel.AttachFolderManager(manager, fixture.filePath("settings.ini"));
            panel.AttachExternalFileServices(&service);
            COpenResult result;
            QCOMPARE(panel.BindToPath(ToUString(archive), UString(), result), S_OK);
            QCOMPARE(panel.GetFolder()->LoadItems(), S_OK);
            panel.OpenItemInArchive(0, tryInternal, true, false, false);
            QVERIFY(!service.Opened.isEmpty());
            QCOMPARE(QFile::exists(service.Opened), !launchFailed);
            QVERIFY(!backend.busy());
            if (changed)
                QVERIFY(write(service.Opened, "changed contents retained"));
        } // Native exit event + join, not an association's presumed completion.
        QCOMPARE(QFile::exists(service.Opened), changed);
        const auto parent = QFileInfo(service.Opened).absolutePath();
        if (changed) {
            QFile edited(service.Opened);
            QVERIFY(edited.open(QIODevice::ReadOnly));
            QCOMPARE(edited.readAll(), QByteArray("changed contents retained"));
            edited.close();
            QVERIFY(QFile::remove(
                service.Opened)); // Fixture owns cleanup of intentionally retained data.
            QVERIFY(QDir().rmdir(parent));
        } else {
            QVERIFY(!QDir(parent).exists());
        }
    }
    void sourceCommandSplit_data() {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("program");
        QTest::addColumn<QString>("params");
        QTest::newRow("quoted-path")
            << QStringLiteral("  \"/папка space/editor\" --readonly  ")
            << QStringLiteral("/папка space/editor") << QString("--readonly");
        QTest::newRow("unquoted-path") << QString("  /path with spaces/editor  ")
                                       << QString("/path with spaces/editor") << QString();
        QTest::newRow("quoted-only") << QString("\"editor\"") << QString("editor") << QString();
        QTest::newRow("unclosed") << QString("\"editor") << QString("\"editor") << QString();
        QTest::newRow("no-delimiter")
            << QString("\"editor\"--flag") << QString("\"editor\"--flag") << QString();
    }
    void sourceCommandSplit() {
        QFETCH(QString, command);
        QFETCH(QString, program);
        QFETCH(QString, params);
        UString nativeProgram, nativeParams;
        SplitCmdLineSmart(ToUString(command), nativeProgram, nativeParams);
        QCOMPARE(ToQString(nativeProgram), program);
        QCOMPARE(ToQString(nativeParams), params);
    }
    void editorViewerSettingsAreIndependent() {
        QTemporaryDir directory;
        const QString path = directory.filePath("settings.ini");
        UString command = L"stale";
        ReadRegEditor(true, command, path);
        QVERIFY(command.IsEmpty());
        const UString editor(L"\"/папка с пробелом/editor\" --edit");
        const UString viewer(L"viewer");
        SaveRegEditor(true, editor, path);
        SaveRegEditor(false, viewer, path);
        ReadRegEditor(true, command, path);
        QVERIFY(command == editor);
        ReadRegEditor(false, command, path);
        QVERIFY(command == viewer);
        SaveRegEditor(true, UString(), path);
        ReadRegEditor(true, command, path);
        QVERIFY(command.IsEmpty());
        ReadRegEditor(false, command, path);
        QVERIFY(command == viewer);
    }
};
QTEST_MAIN(ExternalLaunchTests)
#include "external_launch_tests.moc"
