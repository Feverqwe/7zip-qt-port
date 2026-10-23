#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/ExtractDialogRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "CPP/7zip/UI/GUI/resource2.h"
#include "archivecodeccatalog.h"
#include "copydialog.h"
#include "extractdialog.h"
#include "fixtures/operation_temp_cleanup.h"
#include "fixtures/panel_keys.h"
#include "fixtures/widget_dialogs.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
#include "qtoperationlauncher.h"
#include "source_extract_fixture.h"
#include "window_state_support.h"
#include <QScopeGuard>
#include <QTreeWidget>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <csignal>
#endif

namespace {
using namespace SourceExtractFixture;
QString FixtureTemplate() {
    return QDir(QFileInfo(QDir::tempPath()).canonicalFilePath()).filePath("toolbar-XXXXXX");
}
class GuiTestBackend final : public ArchiveBackend {
  public:
    QList<QtOperationResult> Results;
    QList<QStringList> Selections;
    bool CrashNext = false;
    bool Killed = false;
    QtOperationLauncher *startGuiTest(const QStringList &archives, QString &error) override {
        QtOperationMessage job;
        job.Type = QtOperationMessage::Kind::Job;
        job.Paths = archives;
        job.YesToAll = !CrashNext;
        auto *operation =
            QtOperationLauncher::Launch(QStringLiteral(SEVENZIP_NATIVE_GUI), job, error);
        if (operation) {
            if (CrashNext) {
                CrashNext = false;
                connect(operation, &QtOperationLauncher::accepted, operation, [this, operation] {
#ifdef Q_OS_WIN
                    HANDLE child =
                        OpenProcess(PROCESS_TERMINATE, FALSE, DWORD(operation->processId()));
                    if (child) {
                        Killed = TerminateProcess(child, 0xc0000005u);
                        CloseHandle(child);
                    }
#else
                    Killed = ::kill(pid_t(operation->processId()), SIGKILL) == 0;
#endif
                });
            }
            Selections.append(archives);
            connect(operation, &QtOperationLauncher::finished, this,
                    [this](const QtOperationResult &result) { Results.append(result); });
        }
        return operation;
    }
};
bool makeArchive(const QString &directory, const QString &path, const QString &type,
                 const QString &name) {
    QProcess process;
    process.setWorkingDirectory(directory);
    process.start(QStandardPaths::findExecutable("7zz"), {"a", "-t" + type, "--", path, name});
    return process.waitForFinished(10000) && process.exitCode() == 0;
}
void dismissResults(WidgetDialogFixture &dialogs, QStringList &errors, QStringList &successes) {
    if (auto *widget = dialogs.activeDialog()) {
        if (auto *box = qobject_cast<QMessageBox *>(widget)) {
            // Source ProgressDialog2 success MB_OK carries no icon flags.
            if (box->objectName() == "progressFinalMessage" && box->icon() == QMessageBox::NoIcon)
                successes << box->text();
            else
                errors << box->text();
            box->accept();
        } else if (auto *results = qobject_cast<CListViewDialog *>(widget)) {
            errors << results->windowTitle();
            results->accept();
        } else if (widget->objectName() == "progressDialog") {
            auto *close = widget->findChild<QPushButton *>("IDCANCEL");
            if (close && close->property("languageID").toUInt() == IDS_CLOSE) {
                if (auto *messages = widget->findChild<QTreeWidget *>("IDL_PROGRESS_MESSAGES"))
                    for (int row = 0; row < messages->topLevelItemCount(); ++row)
                        errors << messages->topLevelItem(row)->text(1);
                close->click();
            }
        }
    }
}
} // namespace

class ToolbarArchiveWorkflowTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture dialogs_;
    QString module_;
    quint32 version_ = 0;
    static FileTable *openPanel(MainWindow &window, unsigned pane, const QString &path) {
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *address = ui->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
        return static_cast<FileTable *>(ui->findChild<QTableWidget *>("archiveTable"));
    }
    static void inspectDialog(QDialog &dialog, const QString &tag) {
        for (const QSize size : {QSize(700, 390), QSize(600, 340)}) {
            dialog.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::writeReport(dialog, tag + '-' + QString::number(size.width())));
        }
    }

  private slots:
    void cleanup() { QVERIFY(CleanupOperationTestTemps()); }
    void initTestCase() {
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveCodecCatalog catalog(module_, 0);
        QCOMPARE(catalog.result(), S_OK);
        version_ = catalog.version();
    }
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void repeatedArchiveTesting_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("opened");
        for (const QString &format : {QStringLiteral("zip"), QStringLiteral("7z")})
            for (bool opened : {false, true})
                QTest::newRow(qPrintable(format + (opened ? "-opened" : "-selected")))
                    << format << opened;
    }
    void repeatedArchiveTesting() {
        QFETCH(QString, format);
        QFETCH(bool, opened);
        QTemporaryDir fixture(FixtureTemplate());
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("данные с пробелом.txt");
        const QByteArray payload = QByteArray("archive test payload\n").repeated(10000);
        QVERIFY(writeFile(fixture.filePath(name), payload));
        const QString archive = fixture.filePath("archives/test." + format);
        QVERIFY(QDir().mkpath(QFileInfo(archive).absolutePath()));
        QVERIFY(makeArchive(fixture.path(), archive, format, name));
        const auto before = readFile(archive);
        GuiTestBackend backend;
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *source = openPanel(window, 0, opened ? archive : QFileInfo(archive).absolutePath());
        QTRY_COMPARE(source->rowCount(), 1);
        source->selectRow(0);
        QStringList errors, successes;
        QTimer driver;
        dialogs_.connectResponder(driver, &window,
                                  [&] { dismissResults(dialogs_, errors, successes); });
        driver.start(1);
        for (int attempt = 0; attempt < 3; ++attempt) {
            const bool crash = !opened && attempt == 0;
            backend.CrashNext = crash;
            errors.clear();
            QVERIFY(action(window, "testArchiveAction")->isEnabled());
            source->setFocus();
            PanelKeys::command(source, Qt::Key_T, Qt::ControlModifier | Qt::ShiftModifier);
            QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
            if (opened)
                QCOMPARE(successes.size(), attempt + 1);
            else {
                QVERIFY(action(window, "refreshAction")->isEnabled());
                QTRY_COMPARE_WITH_TIMEOUT(backend.Results.size(), attempt + 1, 30000);
                QCOMPARE(backend.Results.last().Status, crash
                                                            ? QtOperationResult::Outcome::Crashed
                                                            : QtOperationResult::Outcome::Success);
                if (crash) {
                    QVERIFY(backend.Killed);
                    QTRY_VERIFY(!errors.isEmpty());
                } else
                    QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
                QCOMPARE(backend.Selections.last().size(), 1);
                QCOMPARE(QFileInfo(backend.Selections.last().first()).canonicalFilePath(),
                         QFileInfo(archive).canonicalFilePath());
            }
            QCOMPARE(readFile(archive), before);
        }
        if (opened)
            QVERIFY(backend.Selections.isEmpty()); // Live folder testing stays in CopyTo.
    }
    void filesystemToolbarFailureRestoresSource_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::newRow("left") << 0u;
        QTest::newRow("right") << 1u;
    }
    void filesystemToolbarFailureRestoresSource() {
        QFETCH(unsigned, pane);
        QTemporaryDir fixture(FixtureTemplate());
        QVERIFY(fixture.isValid());
        const QString input = fixture.filePath("input");
        const QString archive = QDir(input).filePath(QStringLiteral("сломанный 空.zip"));
        const QByteArray before("invalid private ZIP bytes");
        QVERIFY(writeFile(archive, before));
        const QString output = fixture.filePath("output");
        const QString sentinel = QDir(output).filePath("keep.txt");
        QVERIFY(writeFile(sentinel, "keep"));
        GuiTestBackend backend;
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *source = openPanel(window, pane, input);
        QTRY_COMPARE(source->rowCount(), 1);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        source->selectRow(0);
        source->setCurrentCell(0, 0);
        source->setFocus();
        CMyComPtr<IFolderFolder> provider = source->sourcePanel->GetFolder();
        QStringList errors, successes;
        QTimer driver;
        dialogs_.connectResponder(driver, &window, [&] {
            if (auto *dialog = dynamic_cast<ExtractDialog *>(dialogs_.activeDialog())) {
                dialog->findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
                dialog->findChild<QComboBox *>("extractDestination")->setEditText(output);
                dialog->accept();
            }
            dismissResults(dialogs_, errors, successes);
        });
        driver.start(1);
        for (const char *command : {"extractArchiveAction", "testArchiveAction"}) {
            errors.clear();
            successes.clear();
            QVERIFY(action(window, command)->isEnabled());
            action(window, command)->trigger();
            if (qstrcmp(command, "testArchiveAction") == 0) {
                QTRY_COMPARE_WITH_TIMEOUT(backend.Results.size(), 1, 30000);
                QCOMPARE(backend.Results.last().Status, QtOperationResult::Outcome::Failure);
            } else
                QVERIFY(!errors.isEmpty());
            QVERIFY(successes.isEmpty());
            QCOMPARE(source->sourcePanel->GetFolder(), provider.Interface());
            QCOMPARE(source->selectionModel()->selectedRows().size(), 1);
            QCOMPARE(readFile(archive), before);
            QCOMPARE(readFile(sentinel), QByteArray("keep"));
            QCOMPARE(QDir(output).entryList(QDir::Files), QStringList{"keep.txt"});
            QVERIFY(!backend.busy());
            QVERIFY(source->sourcePanel->ShouldProcessNotifications());
            QVERIFY(action(window, "refreshAction")->isEnabled());
            QTRY_VERIFY(source->hasFocus());
        }
    }
    void archiveToolbarUsesSourceCopyTo_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::newRow("left") << 0u;
        QTest::newRow("right") << 1u;
    }
    void archiveToolbarUsesSourceCopyTo() {
        QFETCH(unsigned, pane);
        QTemporaryDir fixture(FixtureTemplate());
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("данные δ.txt");
        const QByteArray payload("source toolbar bytes");
        QVERIFY(writeFile(fixture.filePath(name), payload));
        const QString archive = fixture.filePath("source.zip");
        QVERIFY(makeArchive(fixture.path(), archive, "zip", name));
        const QByteArray before = readFile(archive);
        const QString output = fixture.filePath("peer output");
        QVERIFY(writeFile(QDir(output).filePath("peer.txt"), "peer bytes"));
        ArchiveBackend backend;
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *peer = openPanel(window, 1 - pane, output);
        QTRY_COMPARE(peer->rowCount(), 1);
        peer->selectRow(0);
        auto *source = openPanel(window, pane, archive);
        QTRY_COMPARE(source->rowCount(), 1);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        source->selectRow(0);
        source->setFocus();
        CMyComPtr<IFolderFolder> provider = source->sourcePanel->GetFolder();
        QString modalName;
        QString destination;
        QTimer driver;
        dialogs_.connectResponder(driver, &window, [&] {
            if (auto *dialog = qobject_cast<QDialog *>(dialogs_.activeDialog())) {
                modalName = dialog->objectName();
                if (auto *copy = dynamic_cast<CopyDialog *>(dialog)) {
                    destination = copy->destination();
                    inspectDialog(*copy, QString("toolbar-archive-copy-%1").arg(pane));
                }
                dialog->reject();
            }
        });
        driver.start(1);
        action(window, "extractArchiveAction")->trigger();
        driver.stop();
        QCOMPARE(modalName, QString("copyDialog"));
        QCOMPARE(QDir::cleanPath(destination), QDir::cleanPath(output));
        QCOMPARE(source->sourcePanel->GetFolder(), provider.Interface());
        QCOMPARE(readFile(archive), before);
        QCOMPARE(readFile(QDir(output).filePath("peer.txt")), QByteArray("peer bytes"));
        QVERIFY(!QFileInfo::exists(QDir(output).filePath(name)));
        QCOMPARE(peer->selectionModel()->selectedRows().size(), 1);
        QVERIFY(source->sourcePanel->ShouldProcessNotifications());
    }
    void filesystemToolbarUsesSourceBatch_data() {
        QTest::addColumn<bool>("multiple");
        QTest::addColumn<unsigned>("pane");
        for (bool multiple : {false, true})
            for (unsigned pane : {0u, 1u})
                QTest::newRow(qPrintable(QString("%1-%2").arg(multiple).arg(pane)))
                    << multiple << pane;
    }
    void filesystemToolbarUsesSourceBatch() {
        QFETCH(bool, multiple);
        QFETCH(unsigned, pane);
        QTemporaryDir fixture(FixtureTemplate());
        QVERIFY(fixture.isValid());
        const QString input = fixture.filePath("archives");
        QVERIFY(QDir().mkpath(input));
        const QString name = QStringLiteral("данные 空.txt");
        const QByteArray payload("batch toolbar bytes");
        QVERIFY(writeFile(fixture.filePath(name), payload));
        const QString first = QDir(input).filePath(QStringLiteral("первый δ.zip"));
        const QString second = QDir(input).filePath("second.7z");
        QVERIFY(makeArchive(fixture.path(), first, "zip", name));
        QVERIFY(makeArchive(fixture.path(), second, "7z", name));
        const QByteArray firstBefore = readFile(first), secondBefore = readFile(second);
        const QString output = fixture.filePath("output δ");
        GuiTestBackend backend;
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *source = openPanel(window, pane, input);
        QTRY_COMPARE(source->rowCount(), 2);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        source->sourcePanel->Set_AutoRefresh_Mode(false);
        int firstRow = -1;
        for (int i = 0; i < source->rowCount(); ++i)
            if (source->item(i, 0)->text().normalized(QString::NormalizationForm_C) ==
                QFileInfo(first).fileName())
                firstRow = i;
        QVERIFY(firstRow >= 0);
        source->selectRow(firstRow);
        source->setCurrentCell(firstRow, 0);
        source->setFocus();
        if (multiple)
            source->sourcePanel->SelectAll(true);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        CMyComPtr<IFolderFolder> provider = source->sourcePanel->GetFolder();
        QString caption;
        bool seen = false;
        bool canceled = true;
        QStringList errors;
        QStringList successes;
        QTimer driver;
        dialogs_.connectResponder(driver, &window, [&] {
            if (auto *dialog = dynamic_cast<ExtractDialog *>(dialogs_.activeDialog())) {
                seen = true;
                caption = dialog->windowTitle();
                if (canceled)
                    dialog->reject();
                else {
                    dialog->findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
                    dialog->findChild<QComboBox *>("extractDestination")
                        ->setEditText(multiple ? output + "/*/" : output);
                    inspectDialog(*dialog, QString("toolbar-batch-%1-%2").arg(multiple).arg(pane));
                    dialog->accept();
                }
            }
            dismissResults(dialogs_, errors, successes);
        });
        driver.start(1);
        source->setFocus();
        PanelKeys::command(source, Qt::Key_E, Qt::ControlModifier | Qt::ShiftModifier);
        QVERIFY(seen);
        QVERIFY(!QFileInfo::exists(output));
        canceled = false;
        seen = false;
        source->setFocus();
        PanelKeys::command(source, Qt::Key_E, Qt::ControlModifier | Qt::ShiftModifier);
        QVERIFY(seen);
        QCOMPARE(caption.normalized(QString::NormalizationForm_C),
                 (QtLangText(IDD_EXTRACT, "Extract") +
                  (multiple ? QString() : " : " + QDir::toNativeSeparators(first)))
                     .normalized(QString::NormalizationForm_C));
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        QCOMPARE(
            readFile(QDir(output).filePath(multiple ? QStringLiteral("первый δ/") + name : name)),
            payload);
        if (multiple)
            QCOMPARE(readFile(QDir(output).filePath("second/" + name)), payload);
        successes.clear();
        source->setFocus();
        PanelKeys::command(source, Qt::Key_T, Qt::ControlModifier | Qt::ShiftModifier);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        QTRY_COMPARE_WITH_TIMEOUT(backend.Results.size(), 1, 30000);
        QCOMPARE(backend.Results.last().Status, QtOperationResult::Outcome::Success);
        QCOMPARE(backend.Selections.last().size(), multiple ? 2 : 1);
        driver.stop();
        QCOMPARE(source->sourcePanel->GetFolder(), provider.Interface());
        QCOMPARE(readFile(first), firstBefore);
        QCOMPARE(readFile(second), secondBefore);
        QCOMPARE(readFile(fixture.filePath(name)), payload);
        QVERIFY(source->sourcePanel->ShouldProcessNotifications());
        QVERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_VERIFY(source->hasFocus());
    }
};
QTEST_MAIN(ToolbarArchiveWorkflowTests)
#include "toolbar_archive_workflow_tests.moc"
