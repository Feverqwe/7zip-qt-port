#include "filemanager/support/window_state_support.h"
#include "filemanager/views/filetable.h"
#include <QElapsedTimer>

class ModalRefreshWindowTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void copyModalRetainsSourceProjectionAcrossTimerTicks() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        const QString source = fixture.filePath("source-panel");
        const QString target = fixture.filePath("other-panel-target");
        QVERIFY(QDir().mkpath(source));
        QVERIFY(QDir().mkpath(target));
        QFile input(QDir(source).filePath(QStringLiteral("данные with spaces.txt")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("source"), qint64(6));
        input.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        action(window, "twoPanelsAction")->trigger();
        auto *left = window.findChild<QWidget *>("filePanel0");
        auto *right = window.findChild<QWidget *>("filePanel1");
        QVERIFY(left && right);
        auto *leftTable = left->findChild<FileTable *>("archiveTable");
        auto *rightTable = right->findChild<FileTable *>("archiveTable");
        const auto navigate = [&](QWidget *panel, FileTable *view, const QString &path) {
            window.activateWindow();
            view->setFocus();
            QCoreApplication::processEvents();
            auto *address = panel->findChild<QLineEdit *>("archivePath");
            address->setText(path);
            QTest::keyClick(address, Qt::Key_Return);
        };
        navigate(right, rightTable, target);
        QTRY_VERIFY(rightTable->isEnabled());
        navigate(left, leftTable, source);
        QTRY_VERIFY(leftTable->isEnabled() && leftTable->rowCount() == 1);
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        leftTable->selectRow(0);
        QVERIFY(action(window, "copyAction")->isEnabled());
        QStringList titles;
        connect(&window, &QWidget::windowTitleChanged, &window,
                [&](const QString &title) { titles.append(title); });
        bool shown = false;
        QString destination;
        QElapsedTimer elapsed;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            auto *dialog = qobject_cast<CopyDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            if (!shown) {
                shown = true;
                destination = dialog->destination();
                elapsed.start();
            }
            // The application timer runs every second. Keep the actual modal
            // open through two ticks, observing title/projection publicly.
            if (elapsed.elapsed() >= 2100)
                dialog->reject();
        });
        answer.start(5);
        action(window, "copyAction")->trigger();
        answer.stop();
        QVERIFY(shown);
        QCOMPARE(QDir::cleanPath(destination), QDir::cleanPath(target));
        for (const QString &title : titles)
            QVERIFY2(!title.contains("other-panel-target"), qPrintable(title));
        QCOMPARE(leftTable->selectionModel()->selectedRows().size(), 1);
        QVERIFY2(QDir::fromNativeSeparators(window.windowTitle())
                     .startsWith(QDir::fromNativeSeparators(source)),
                 qPrintable(window.windowTitle()));
        QVERIFY(QFileInfo::exists(input.fileName()));
        QVERIFY(!QFileInfo::exists(QDir(target).filePath(QFileInfo(input).fileName())));
    }
};
QTEST_MAIN(ModalRefreshWindowTests)
#include "modal_refresh_window_tests.moc"
