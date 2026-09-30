#include "copydialog.h"
#include "window_state_support.h"

class UiCopySameTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void shiftF5CopiesFocusedFileWithNewName() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath(QStringLiteral("исходный файл.txt"));
        const QString target = fixture.filePath(QStringLiteral("новое имя.txt"));
        QFile input(source);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("source bytes"), qint64(12));
        input.close();

        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        QVERIFY(address);
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        window.show();
        QCoreApplication::processEvents();
        auto *view = table(window);
        QTRY_VERIFY(view->rowCount() > 0);
        const int row = filesystemRow(window, QStringLiteral("исходный файл.txt"));
        QVERIFY(row >= 0);
        view->selectRow(row);
        view->setCurrentCell(row, 0);
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        bool dialogSeen = false;
        QString initial;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (!dialog)
                return;
            dialogSeen = true;
            auto *destination = dialog->findChild<QComboBox *>("copyDestination");
            initial = destination->currentText();
            destination->setCurrentText(QStringLiteral("новое имя.txt"));
            dialog->accept();
        });
        QTest::keyClick(view, Qt::Key_F5, Qt::ShiftModifier);
        QVERIFY(dialogSeen);
        QCOMPARE(initial.normalized(QString::NormalizationForm_C),
                 QStringLiteral("исходный файл.txt"));
        QTRY_VERIFY(QFileInfo::exists(target));
        QFile copied(target);
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("source bytes"));
        QVERIFY(QFileInfo::exists(source));
    }

    void shiftF6MovesFocusedFileToNewName() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath(QStringLiteral("первое имя.txt"));
        const QString target = fixture.filePath(QStringLiteral("второе имя.txt"));
        QFile input(source);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("move bytes"), qint64(10));
        input.close();

        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        QVERIFY(address);
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        window.show();
        QCoreApplication::processEvents();
        auto *view = table(window);
        QTRY_VERIFY(view->rowCount() > 0);
        const int row = filesystemRow(window, QStringLiteral("первое имя.txt"));
        QVERIFY(row >= 0);
        view->selectRow(row);
        view->setCurrentCell(row, 0);
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        bool dialogSeen = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (!dialog)
                return;
            dialogSeen = true;
            dialog->findChild<QComboBox *>("copyDestination")
                ->setCurrentText(QStringLiteral("второе имя.txt"));
            dialog->accept();
        });
        QTest::keyClick(view, Qt::Key_F6, Qt::ShiftModifier);
        QVERIFY(dialogSeen);
        QTRY_VERIFY(QFileInfo::exists(target));
        QTRY_VERIFY(!QFileInfo::exists(source));
        QFile moved(target);
        QVERIFY(moved.open(QIODevice::ReadOnly));
        QCOMPARE(moved.readAll(), QByteArray("move bytes"));
    }

    void ordinaryF5AcceptsSingleFilenameDestination() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath(QStringLiteral("source file.txt"));
        QFile input(source);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("ordinary bytes"), qint64(14));
        input.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        window.show();
        QCoreApplication::processEvents();
        auto *view = table(window);
        const int row = filesystemRow(window, QStringLiteral("source file.txt"));
        QVERIFY(row >= 0);
        view->selectRow(row);
        view->setCurrentCell(row, 0);
        view->setFocus();
        const QString target = fixture.filePath(QStringLiteral("nested/new file.txt"));
        bool dialogSeen = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (!dialog)
                return;
            dialogSeen = true;
            dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(target);
            dialog->accept();
        });
        QTest::keyClick(view, Qt::Key_F5);
        QVERIFY(dialogSeen);
        QTRY_VERIFY(QFileInfo::exists(target));
        QFile copied(target);
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("ordinary bytes"));
        QVERIFY(QFileInfo::exists(source));
    }

    void shiftF5CopiesFocusedArchiveItemToAbsoluteFolder() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString first = fixture.filePath(QStringLiteral("first file.txt"));
        const QString second = fixture.filePath(QStringLiteral("second file.txt"));
        for (const QString &path : {first, second}) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(QFileInfo(path).fileName().toUtf8()),
                     qint64(QFileInfo(path).fileName().toUtf8().size()));
        }
        const QString archive = fixture.filePath(QStringLiteral("archive.zip"));
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {first, second});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        window.openArchive(archive);
        window.show();
        auto *view = table(window);
        QTRY_COMPARE(view->rowCount(), 2);
        const int focused = filesystemRow(window, QStringLiteral("first file.txt"));
        const int selected = filesystemRow(window, QStringLiteral("second file.txt"));
        QVERIFY(focused >= 0);
        QVERIFY(selected >= 0);
        view->selectRow(selected);
        view->setCurrentCell(focused, 0, QItemSelectionModel::NoUpdate);
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        const QString destination = fixture.filePath(QStringLiteral("папка назначения"));
        bool dialogSeen = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (!dialog)
                return;
            dialogSeen = true;
            dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(destination);
            dialog->accept();
        });
        QTest::keyClick(view, Qt::Key_F5, Qt::ShiftModifier);
        QVERIFY(dialogSeen);
        QTRY_VERIFY(
            QFileInfo::exists(QDir(destination).filePath(QStringLiteral("first file.txt"))));
        QVERIFY(!QFileInfo::exists(QDir(destination).filePath(QStringLiteral("second file.txt"))));

        const QString movedDestination = fixture.filePath(QStringLiteral("moved folder"));
        view->setCurrentCell(selected, 0);
        view->setFocus();
        dialogSeen = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (!dialog)
                return;
            dialogSeen = true;
            dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(movedDestination);
            dialog->accept();
        });
        QTest::keyClick(view, Qt::Key_F6, Qt::ShiftModifier);
        QVERIFY(dialogSeen);
        QTRY_VERIFY(
            QFileInfo::exists(QDir(movedDestination).filePath(QStringLiteral("second file.txt"))));
        QTRY_COMPARE(view->rowCount(), 1);
        QCOMPARE(view->item(0, 0)->text(), QStringLiteral("first file.txt"));
    }
};

QTEST_MAIN(UiCopySameTests)
#include "ui_copy_same_tests.moc"
