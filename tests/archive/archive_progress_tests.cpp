#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "archive/support/archive_backend_test_support.h"
#include "archive/support/native_archive_fixture.h"
#include "dialogs/password/PasswordDialog.h"
#include "platform/process/qtexternalprocess.h"
#include "support/progress_dialog.h"
#include "support/widget_dialogs.h"
#include "ui/progress/archiveprogressoperation.h"
#include "ui/progress/archiveprogresstransport.h"
#include "ui/support/layout_inspection.h"
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
        emit backend.phaseStarted();
        QCOMPARE(backend.Paused, !failure);
        QCOMPARE(backend.Cancelled, failure);
        QCOMPARE(dialog.Sync.Messages.Size(), unsigned(failure ? 1 : 0));
        QVERIFY(dialog.Sync.Get_Paused());
        if (!failure) {
            QCOMPARE(transport.SetPaused(false), S_OK);
            QVERIFY(!backend.Paused);
        }
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
        QVERIFY(NativeArchiveFixture::Write(archive, {{"payload.txt"}}, "7z",
                                            empty ? QStringLiteral("")
                                                  : QStringLiteral("fixture пароль")));
        ArchiveBackend backend;
        ArchiveProgressOperation operation(backend,
                                           [&] { backend.testSourceArchive(archive, {}); });
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
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        ArchiveProgressOperation operation(backend, [&] {
            if (afterStart)
                backend.hashMethods({source}, {"CRC32"});
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
        QCOMPARE(operation.Result, HRESULT(E_FAIL));
        QCOMPARE(observed, QStringLiteral("Ошибка запуска worker"));
        QVERIFY(operation.ThreadFinishedOK);
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(archive));
        QVERIFY(QDir(directory.path())
                    .entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden)
                    .isEmpty());
    }
};

QTEST_MAIN(ArchiveProgressTests)
#include "archive_progress_tests.moc"
