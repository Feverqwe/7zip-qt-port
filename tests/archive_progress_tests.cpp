#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "archive_backend_test_support.h"
#include "archiveprogressoperation.h"
#include "archiveprogresstransport.h"
#include "fixtures/progress_dialog.h"
#include "fixtures/widget_dialogs.h"
#include "layout_inspection.h"
#include "qtexternalprocess.h"
#include <QApplication>
#include <QDeadlineTimer>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QScopeGuard>

namespace {
class CControlledTransport : public ArchiveBackend {
  public:
    bool Active = false;
    bool Paused = false;
    bool Cancelled = false;
    HRESULT PauseResult = S_OK;
    bool busy() const override { return Active; }
    HRESULT setPaused(bool value) override {
        if (PauseResult == S_OK)
            Paused = value;
        return PauseResult;
    }
    void cancel() override { Cancelled = true; }
    void Start() {
        Active = true;
        emit progress(42, QStringLiteral("папка/файл с пробелом.txt"));
    }
    void Finish() {
        Active = false;
        emit finished(true, QStringLiteral("Готово"));
    }
};
} // namespace

class ArchiveProgressTests : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT
    WidgetDialogFixture widgetDialogs_;
  private slots:
    void newEngineRetainsPauseOrReportsFailure_data() {
        QTest::addColumn<bool>("failure");
        QTest::newRow("paused") << false;
        QTest::newRow("pause-failure") << true;
    }
    void newEngineRetainsPauseOrReportsFailure() {
        QFETCH(bool, failure);
        CControlledTransport backend;
        CProgressDialog dialog;
        ArchiveProgressTransport transport(dialog, backend);
        backend.Active = true;
        dialog.Sync.Set_Paused(true);
        backend.PauseResult = failure ? E_FAIL : S_OK;
        emit backend.engineStarted();
        QCOMPARE(backend.Paused, !failure);
        QCOMPARE(backend.Cancelled, failure);
        QCOMPARE(dialog.Sync.Messages.Size(), unsigned(failure ? 1 : 0));
        QVERIFY(dialog.Sync.Get_Paused());
        if (!failure) {
            QCOMPARE(transport.SetPaused(false), S_OK);
            QVERIFY(!backend.Paused);
        }
    }
    void creationPhasesRetainPause_data() {
        QTest::addColumn<int>("pausedPhase");
        QTest::newRow("scan") << 1;
        QTest::newRow("engine") << 2;
        QTest::newRow("volumes") << 3;
        QTest::newRow("delete-inputs") << 4;
    }
    void creationPhasesRetainPause() {
        QFETCH(int, pausedPhase);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath(QStringLiteral("источник с пробелом.txt"));
        const QString archive = fixture.filePath(QStringLiteral("архив с пробелом.7z"));
        writeFile(source, QByteArray(1024 * 1024, 'x'));
        ArchiveBackend backend;
        CProgressDialog dialog;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        int phase = 0;
        bool observedPause = false;
        // Model an outstanding pause at the transition, before the transport
        // binds the new process/worker to the existing operation controls.
        connect(&backend, &ArchiveBackend::phaseStarted, &dialog, [&] {
            if (++phase != pausedPhase)
                return;
            dialog.Sync.Set_Paused(true);
            QTimer::singleShot(150, &dialog, [&] {
                observedPause = finished.isEmpty() && QFileInfo::exists(source);
                if (pausedPhase == 3)
                    observedPause = observedPause && !QFileInfo::exists(archive + ".001");
                dialog.Sync.Set_Paused(false);
                QCOMPARE(backend.setPaused(false), S_OK);
            });
        });
        ArchiveProgressTransport transport(dialog, backend);
        CompressionOptions options;
        options.DeleteAfterCompressing = true;
        options.VolumeSizes.Add(1024 * 1024);
        backend.create(archive, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY(observedPause);
        QCOMPARE(phase, 4);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QVERIFY(QFileInfo::exists(archive + ".001"));
        QVERIFY(!QFileInfo::exists(source));
        QVERIFY(!backend.busy());
    }
    void controlsAfterChildExitPreserveOperationResult_data() {
        QTest::addColumn<int>("exitCode");
        QTest::newRow("success") << 0;
        QTest::newRow("failure") << 2;
    }
    void controlsAfterChildExitPreserveOperationResult() {
        QFETCH(int, exitCode);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath(QStringLiteral("архив с пробелом.zip"));
        writeFile(archive, "process control fixture");
        qputenv("SEVENZIP_TEST_HELPER", "control-exit");
        qputenv("SEVENZIP_CONTROL_FIXTURE", fixture.path().toUtf8());
        qputenv("SEVENZIP_CONTROL_EXIT_CODE", QByteArray::number(exitCode));
        const auto restore = qScopeGuard([] {
            qunsetenv("SEVENZIP_TEST_HELPER");
            qunsetenv("SEVENZIP_CONTROL_FIXTURE");
            qunsetenv("SEVENZIP_CONTROL_EXIT_CODE");
        });
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        bool exited = false;
        HRESULT pause = E_FAIL;
        connect(&backend, &ArchiveBackend::engineStarted, &backend, [&] {
            // Keep Qt's completion notification pending while the OS child exits.
            QFile pidFile(fixture.filePath("pid"));
            QDeadlineTimer deadline(5000);
            while (!pidFile.open(QIODevice::ReadOnly) && !deadline.hasExpired())
                QThread::msleep(1);
            const UInt32 pid = pidFile.readAll().toUInt();
            QVERIFY(pid != 0);
            QtNativeProcessServices services;
            std::unique_ptr<IQtProcessHandle> child(services.OpenProcess(pid));
            QVERIFY(child);
            writeFile(fixture.filePath("exit"), "release");
            while (child->Status() == QtProcessStatus::Running && !deadline.hasExpired())
                QThread::msleep(1);
            exited = child->Status() == QtProcessStatus::Exited;
            QVERIFY(exited);
            pause = backend.setPaused(true);
        });
        backend.testSourceArchive(archive, {});
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(exited);
        QCOMPARE(pause, S_OK);
        QCOMPARE(finished[0][0].toBool(), exitCode == 0);
        QCOMPARE(backend.lastResult(), exitCode == 0 ? S_OK : E_FAIL);
        QVERIFY(!backend.busy());
    }
    void ownedEnginePasswordCallback_data() {
        QTest::addColumn<bool>("accept");
        QTest::addColumn<bool>("empty");
        QTest::newRow("accept") << true << false;
        QTest::newRow("empty") << true << true;
        QTest::newRow("cancel") << false << false;
    }
    void ownedEnginePasswordCallback() {
        QFETCH(bool, accept);
        QFETCH(bool, empty);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.filePath(QStringLiteral("fixture.7z"));
        writeFile(archive, "test fixture");
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        backend.list(archive);
        await(backend, true);
        qputenv("SEVENZIP_TEST_HELPER", empty ? "progress-password-empty" : "progress-password");
        ArchiveProgressOperation operation(backend, [&] { backend.test(archive); });
        bool observed = false;
        QTimer responder;
        responder.setInterval(5);
        widgetDialogs_.connectResponder(responder, &operation, [&] {
            auto *dialog = dynamic_cast<CPasswordDialog *>(widgetDialogs_.activeDialog());
            if (!dialog || observed)
                return;
            observed = true;
            dialog->findChild<QLineEdit *>("passwordEdit")
                ->setText(empty ? QString() : QStringLiteral("fixture пароль"));
            if (accept)
                dialog->accept();
            else
                dialog->reject();
        });
        responder.start();
        QCOMPARE(operation.Create(L"Пароль"), HRESULT(S_OK));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(observed);
        QCOMPARE(operation.Result, HRESULT(accept ? S_OK : E_ABORT));
        QVERIFY(!backend.busy());
    }
    void transportControlsAndFinishWhilePaused_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(760, 450);
        QTest::newRow("compact") << QSize(640, 380);
    }
    void transportControlsAndFinishWhilePaused() {
        QFETCH(QSize, size);
        CControlledTransport backend;
        ArchiveProgressOperation operation(backend, [&] { backend.Start(); });
        operation.resize(size);
        unsigned stage = 0;
        bool geometry = false;
        QTimer responder;
        responder.setInterval(5);
        widgetDialogs_.connectResponder(responder, &operation, [&] {
            auto *pause = operation.findChild<QPushButton *>("IDB_PAUSE");
            auto *close = operation.findChild<QPushButton *>("IDCANCEL");
            if (stage == 0 && widgetDialogs_.ready(operation) &&
                operation.findChild<QProgressBar *>("IDC_PROGRESS1")->value() == 42 &&
                operation.windowTitle().contains(QStringLiteral("42%"))) {
                stage = 1;
                QCOMPARE(operation.Sync._totalBytes, UInt64(-1));
                QCOMPARE(operation.Sync._completedBytes, UInt64(0));
                QVERIFY(operation.findChild<QLabel *>("IDT_PROGRESS_TOTAL_VAL")->text().isEmpty());
                QVERIFY(operation.windowTitle().contains(QStringLiteral("42%")));
                QVERIFY(!operation.findChild<QPushButton *>("IDB_PROGRESS_BACKGROUND"));
                QCOMPARE(operation.Sync.Messages.Size(), 0u);
                backend.PauseResult = E_FAIL;
                pause->click();
                QVERIFY(!operation.Sync.Get_Paused());
                QVERIFY(!backend.Paused);
                QCOMPARE(pause->text(), QStringLiteral("&Pause"));
                backend.PauseResult = S_OK;
                pause->click();
                QVERIFY(operation.Sync.Get_Paused());
                QVERIFY(backend.Paused);
                // External I/O may finish after a pause request. Completion must
                // not strand the bridge in cooperative CheckStop.
                backend.Finish();
            } else if (stage == 1 && TestProgressDialog::IsCloseButton(close)) {
                stage = 2;
                geometry =
                    operation.size() == size && close->isDefault() &&
                    UiInspection::fullyVisible(*close, operation) && !pause->isVisible() &&
                    UiInspection::writeReport(
                        operation, QString("archive-progress-%1-controls-close").arg(size.width()));
                close->click();
            }
        });
        responder.start();
        QCOMPARE(operation.Create(L"Добавление"), HRESULT(S_OK));
        responder.stop();
        QCOMPARE(stage, unsigned(2));
        QVERIFY(geometry);
        QCOMPARE(operation.Result, HRESULT(S_OK));
        QCOMPARE(operation.Sync.Messages.Size(), 1u);
        QVERIFY(!backend.busy());
    }
    void startExceptionUsesSourceWorkerComposition_data() {
        QTest::addColumn<bool>("afterStart");
        QTest::newRow("before-start") << false;
        QTest::newRow("after-start") << true;
    }
    void startExceptionUsesSourceWorkerComposition() {
        QFETCH(bool, afterStart);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = directory.filePath(QStringLiteral("источник с пробелом.txt"));
        const QString archive = directory.filePath("unfinished.7z");
        writeFile(source, "original");
        qputenv("SEVENZIP_TEST_HELPER", "createpartial");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        ArchiveProgressOperation operation(backend, [&] {
            if (afterStart)
                backend.create(archive, {source});
            throw UString(L"Ошибка запуска worker");
        });
        QString observed;
        QTimer responder;
        responder.setInterval(5);
        widgetDialogs_.connectResponder(responder, &operation, [&] {
            auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog());
            if (box && box->objectName() == "progressFinalMessage" && observed.isEmpty()) {
                observed = box->text();
                box->button(QMessageBox::Ok)->click();
            }
        });
        responder.start();
        QCOMPARE(operation.Create(L"Запуск"), HRESULT(S_OK));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QCOMPARE(operation.Result, HRESULT(E_FAIL));
        QCOMPARE(observed, QStringLiteral("Ошибка запуска worker"));
        QVERIFY(operation.ThreadFinishedOK);
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(archive));
        QVERIFY(QDir(directory.path())
                    .entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden)
                    .isEmpty());
    }
    void realArchiveAdd_data() {
        QTest::addColumn<bool>("valid");
        QTest::newRow("success") << true;
        QTest::newRow("missing-source") << false;
    }
    void realArchiveAdd() {
        QFETCH(bool, valid);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString old = directory.filePath(QStringLiteral("старый файл.txt"));
        const QString added = directory.filePath(QStringLiteral("новый файл.txt"));
        const QString archive = directory.filePath(QStringLiteral("архив с пробелом.zip"));
        writeFile(old, "old");
        writeFile(added, "added");
        QProcess engine;
        engine.start(QStringLiteral("7zz"), {"a", "-tzip", "--", archive, old});
        QVERIFY(engine.waitForFinished(10000));
        QCOMPARE(engine.exitCode(), 0);
        QFile before(archive);
        QVERIFY(before.open(QIODevice::ReadOnly));
        const QByteArray original = before.readAll();
        before.close();
        ArchiveBackend backend;
        backend.list(archive);
        await(backend, true);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        ArchiveProgressOperation operation(backend, [&] {
            backend.add(archive, {valid ? added : directory.filePath("missing файл.txt")});
        });
        bool errorObserved = false;
        QString errorText;
        QTimer responder;
        responder.setInterval(5);
        widgetDialogs_.connectResponder(responder, &operation, [&] {
            auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog());
            if (box && box->objectName() == "progressFinalMessage" && !errorObserved) {
                errorObserved = true;
                errorText = box->text();
                box->button(QMessageBox::Ok)->click();
            }
            if (box)
                return; // Let source finish leave the message's nested event loop.
            auto *close = operation.findChild<QPushButton *>("IDCANCEL");
            if (operation.isVisible() && widgetDialogs_.activeDialog() == &operation && close &&
                TestProgressDialog::IsCloseButton(close))
                close->click();
        });
        responder.start();
        QCOMPARE(operation.Create(L"Добавление"), HRESULT(S_OK));
        responder.stop();
        QCOMPARE(operation.Result, HRESULT(valid ? S_OK : E_FAIL));
        QCOMPARE(operation.succeeded(), valid);
        QCOMPARE(errorObserved, !valid);
        if (!valid)
            QVERIFY2(errorText.contains(QStringLiteral("missing файл.txt")), qPrintable(errorText));
        QCOMPARE(finished.size(), 1);
        QVERIFY2(operation.Sync.Messages.IsEmpty(),
                 operation.Sync.Messages.IsEmpty()
                     ? ""
                     : qPrintable(QString::fromWCharArray(operation.Sync.Messages[0].Ptr())));
        QVERIFY(!backend.busy());
        if (valid) {
            engine.start(QStringLiteral("7zz"), {"l", "-sccUTF-8", "--", archive});
            QVERIFY(engine.waitForFinished(10000));
            QCOMPARE(engine.exitCode(), 0);
            const QString listing = QString::fromUtf8(engine.readAllStandardOutput())
                                        .normalized(QString::NormalizationForm_C);
            QVERIFY(listing.contains(QStringLiteral("старый файл.txt")));
            QVERIFY(listing.contains(QStringLiteral("новый файл.txt")));
        } else {
            QVERIFY(before.open(QIODevice::ReadOnly));
            QCOMPARE(before.readAll(), original);
        }
    }
    void cancelPausedOwnedEngine() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = directory.filePath(QStringLiteral("файл с пробелом.txt"));
        const QString archive = directory.filePath(QStringLiteral("архив.7z"));
        writeFile(source, "original");
        const QString oldSource = directory.filePath("old.txt");
        writeFile(oldSource, "old");
        ArchiveBackend real;
        real.create(archive, {oldSource});
        await(real, true);
        QFile original(archive);
        QVERIFY(original.open(QIODevice::ReadOnly));
        const QByteArray saved = original.readAll();
        original.close();
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        backend.list(archive);
        await(backend, true);
        qputenv("SEVENZIP_TEST_HELPER", "createpartial");
        ArchiveProgressOperation operation(backend, [&] { backend.add(archive, {source}); });
        bool started = false, paused = false, confirmed = false;
        connect(&backend, &ArchiveBackend::outputReceived, &operation,
                [&](const QString &) { started = true; });
        QTimer responder;
        responder.setInterval(5);
        widgetDialogs_.connectResponder(responder, &operation, [&] {
            auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog());
            if (box && box->objectName() == "progressCancelQuestion") {
                confirmed = true;
                QVERIFY(operation.Sync.Get_Paused());
                box->button(QMessageBox::Yes)->click();
                return;
            }
            if (!started || paused)
                return;
            paused = true;
            operation.findChild<QPushButton *>("IDB_PAUSE")->click();
            QVERIFY(operation.Sync.Get_Paused());
            QTimer::singleShot(0, &operation, [&] { operation.reject(); });
        });
        responder.start();
        QCOMPARE(operation.Create(L"Добавление"), HRESULT(S_OK));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(started);
        QVERIFY(paused);
        QVERIFY(confirmed);
        QCOMPARE(operation.Result, HRESULT(E_ABORT));
        QVERIFY(!backend.busy());
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), saved);
        QVERIFY(QDir(directory.path())
                    .entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden)
                    .isEmpty());
    }
};

int main(int argc, char **argv) {
    const QByteArray helperMode = qgetenv("SEVENZIP_TEST_HELPER");
    if (helperMode == "control-exit") {
        QCoreApplication app(argc, argv);
        const QDir fixture(qEnvironmentVariable("SEVENZIP_CONTROL_FIXTURE"));
        QSaveFile pid(fixture.filePath("pid"));
        if (!pid.open(QIODevice::WriteOnly))
            return 3;
        pid.write(QByteArray::number(QCoreApplication::applicationPid()));
        if (!pid.commit())
            return 3;
        QDeadlineTimer deadline(5000);
        while (!QFileInfo::exists(fixture.filePath("exit")) && !deadline.hasExpired())
            QThread::msleep(1);
        return deadline.hasExpired() ? 3
                                     : qEnvironmentVariableIntValue("SEVENZIP_CONTROL_EXIT_CODE");
    }
    if (helperMode.startsWith("progress-password")) {
        QCoreApplication app(argc, argv);
        writeOutput("Enter password:");
        char input[256];
        if (!std::fgets(input, sizeof(input), stdin))
            return 2;
        const QByteArray expected = helperMode == "progress-password-empty"
                                        ? QByteArray()
                                        : QStringLiteral("fixture пароль").toUtf8();
        return QByteArray(input).trimmed() == expected ? 0 : 2;
    }
    if (!qgetenv("SEVENZIP_TEST_HELPER").isEmpty()) {
        QObject unused;
        return backendTestMain(argc, argv, &unused);
    }
    QApplication app(argc, argv);
    ArchiveProgressTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "archive_progress_tests.moc"
