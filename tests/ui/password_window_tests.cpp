#include "dialogs/password/PasswordDialog.h"
#include "filemanager/support/window_state_support.h"
#include "ui/support/layout_inspection.h"
#include <QScopeGuard>

class PasswordWindowTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void passwordPromptCancellationAndReopen() {
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(backend.prepareArchiveFixture("protected.7z"));
        QTimer responder;
        connect(&responder, &QTimer::timeout, &window, [] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (dialog && dialog->objectName() == "passwordDialog")
                dialog->reject();
        });
        responder.start(1);
        backend.queueListAction([&] { emit backend.passwordRequired(); });
        window.openArchive("protected.7z");
        responder.stop();
        QCOMPARE(backend.cancelCalls, 1);
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        openListedArchive(window, backend, "empty.zip", {});
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        action(window, "extractArchiveAction")->trigger();
        QVERIFY(!QApplication::activeModalWidget());
        QCOMPARE(backend.extractCalls, 0);
    }

    void passwordReuseAndResetOnReopen_data() {
        QTest::addColumn<QString>("password");
        QTest::newRow("unicode") << QStringLiteral("disposable-δ-password");
        QTest::newRow("spaces") << QStringLiteral("  пароль 密碼  ");
        QTest::newRow("empty") << QString();
    }
    void passwordReuseAndResetOnReopen() {
        QFETCH(QString, password);
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(backend.prepareArchiveFixture("protected.7z"));
        QTimer responder;
        bool supplied = false;
        connect(&responder, &QTimer::timeout, &window, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "passwordDialog" || supplied)
                return;
            supplied = true;
            const auto finish = qScopeGuard([&] {
                dialog->accept();
                QTimer::singleShot(0, &backend, [&] {
                    emit backend.infoReady(
                        ArchiveInfo{QStringLiteral("7z"), 120, 0, 3, 1, 0, true});
                    backend.completeList({ArchiveEntry{QStringLiteral("δ file.txt"), 3, 3}});
                });
            });
            auto *edit = dialog->findChild<QLineEdit *>("passwordEdit");
            QVERIFY(edit);
            edit->setText(password);
            auto *show = dialog->findChild<QCheckBox *>("passwordShow");
            QVERIFY(show);
            if (!show->isChecked())
                show->click();
            QCOMPARE(edit->echoMode(), QLineEdit::Normal);
            for (int width : {360, 260}) {
                dialog->resize(width, 170);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*edit, *dialog));
                QVERIFY(UiInspection::fullyVisible(*show, *dialog));
                const auto report = UiInspection::dump(*dialog);
                if (!password.isEmpty())
                    QVERIFY(!report.contains(password));
                QVERIFY(UiInspection::writeReport(
                    *dialog,
                    QString("password-window-%1-%2").arg(QTest::currentDataTag()).arg(width)));
            }
        });
        responder.start(1);
        backend.queueListAction([&] { emit backend.passwordRequired(); });
        window.openArchive("protected.7z");
        responder.stop();
        QVERIFY(supplied);
        QCOMPARE(backend.cancelCalls, 0);
        QVERIFY(!action(window, "toolbarAddAction")->isEnabled());
        window.testArchive();
        QCOMPARE(backend.lastTestPassword, password);
        bool transportPrompt = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = dynamic_cast<CPasswordDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            transportPrompt = true;
            dialog->findChild<QLineEdit *>("passwordEdit")->setText(password);
            dialog->accept();
        });
        // This asynchronous process prompt has no source modal owner. Opening
        // above instead routes the same signal through COpenArchiveCallback.
        emit backend.passwordRequired();
        QVERIFY(transportPrompt);
        QCOMPARE(backend.cancelCalls, 0);
        QVERIFY(!action(window, "openInsideAction")->isEnabled());
        backend.fail(QStringLiteral("Операция отменена."), E_ABORT);
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        openListedArchive(window, backend, "empty.zip", {});
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        action(window, "extractArchiveAction")->trigger();
        QVERIFY(!QApplication::activeModalWidget());
        QCOMPARE(backend.extractCalls, 0);
        window.testArchive();
        QVERIFY(backend.lastTestPassword.isEmpty());
    }
};
QTEST_MAIN(PasswordWindowTests)
#include "password_window_tests.moc"
