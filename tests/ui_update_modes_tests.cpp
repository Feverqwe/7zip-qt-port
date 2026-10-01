#include "createarchivedialog.h"
#include "window_state_support.h"
#include <QProcess>

namespace {
bool runEngine(const QString &directory, const QStringList &arguments,
               QByteArray *output = nullptr) {
    QProcess process;
    process.setWorkingDirectory(directory);
    process.start(QStringLiteral("7zz"), arguments);
    if (!process.waitForFinished(30000) || process.exitStatus() != QProcess::NormalExit ||
        process.exitCode() != 0)
        return false;
    if (output)
        *output = process.readAllStandardOutput();
    return true;
}
} // namespace

class UiUpdateModesTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void protectedAddUsesOpenedArchivePassword() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source");
        const QString archive = fixture.filePath("protected.7z");
        const QString added = QDir(root).filePath("added.txt");
        QVERIFY(QDir().mkpath(root));
        QFile keep(QDir(root).filePath("keep.txt"));
        QVERIFY(keep.open(QIODevice::WriteOnly));
        QCOMPARE(keep.write("old"), qint64(3));
        keep.close();
        QVERIFY(
            runEngine(root, {"a", "-t7z", "-pfixture123", "-mhe=on", "--", archive, "keep.txt"}));
        QFile source(added);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write("new"), qint64(3));
        source.close();
        ArchiveBackend backend;
        MainWindow window(&backend);
        int passwordPrompts = 0;
        QTimer passwordAnswer;
        connect(&passwordAnswer, &QTimer::timeout, &window, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "passwordDialog")
                return;
            ++passwordPrompts;
            dialog->findChild<QLineEdit *>("passwordEdit")->setText("fixture123");
            dialog->accept();
        });
        passwordAnswer.start(1);
        window.openArchive(archive);
        window.show();
        QTRY_COMPARE(table(window)->rowCount(), 1);
        bool accepted = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CreateArchiveDialog *>("createArchiveDialog");
            if (!dialog)
                return;
            auto *directory = dialog->findChild<QLineEdit *>("sourceDirectory");
            auto *available = dialog->findChild<QListWidget *>("availableSources");
            if (!directory || !available) {
                dialog->reject();
                return;
            }
            directory->setText(root);
            QMetaObject::invokeMethod(directory, "returnPressed", Qt::DirectConnection);
            for (int row = 0; row < available->count(); ++row)
                if (QFileInfo(available->item(row)->data(Qt::UserRole).toString()).fileName() ==
                    "added.txt")
                    available->item(row)->setSelected(true);
            if (available->selectedItems().size() != 1) {
                dialog->reject();
                return;
            }
            dialog->findChild<QPushButton *>("selectSourcesButton")->click();
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            accepted = dialog->result() == QDialog::Accepted;
        });
        action(window, "addArchiveAction")->trigger();
        QVERIFY(accepted);
        QTRY_COMPARE(table(window)->rowCount(), 2);
        QCOMPARE(passwordPrompts, 1);
        passwordAnswer.stop();
        QByteArray listing;
        QVERIFY(runEngine(fixture.path(), {"l", "-slt", "-pfixture123", "--", archive}, &listing));
        QVERIFY(listing.contains("Path = added.txt"));
        QCOMPARE(listing.count("Encrypted = +"), 2);
    }

    void addDialogUpdatesExistingEntryWithSelectedCompression() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath(QStringLiteral("source file.txt"));
        const QString archive = fixture.filePath("update.zip");
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("old bytes"), qint64(9));
        file.close();
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write("new bytes"), qint64(9));
        QVERIFY(file.setFileTime(QDateTime::currentDateTimeUtc().addSecs(10),
                                 QFileDevice::FileModificationTime));
        file.close();
        MainWindow window(&backend);
        window.openArchive(archive);
        window.show();
        QTRY_COMPARE(table(window)->rowCount(), 1);
        bool dialogSeen = false;
        bool sourceSelected = false;
        bool dialogAccepted = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CreateArchiveDialog *>("createArchiveDialog");
            if (!dialog)
                return;
            dialogSeen = true;
            auto *directory = dialog->findChild<QLineEdit *>("sourceDirectory");
            auto *available = dialog->findChild<QListWidget *>("availableSources");
            auto *add = dialog->findChild<QPushButton *>("selectSourcesButton");
            auto *mode = dialog->findChild<QComboBox *>("archiveUpdateMode");
            auto *level = dialog->findChild<QComboBox *>("compressionLevel");
            if (!directory || !available || !add || !mode || !level) {
                dialog->reject();
                return;
            }
            directory->setText(fixture.path());
            QMetaObject::invokeMethod(directory, "returnPressed", Qt::DirectConnection);
            for (int row = 0; row < available->count(); ++row)
                if (QFileInfo(available->item(row)->data(Qt::UserRole).toString()).fileName() ==
                    QStringLiteral("source file.txt"))
                    available->item(row)->setSelected(true);
            if (available->selectedItems().size() != 1) {
                dialog->reject();
                return;
            }
            sourceSelected = true;
            add->click();
            mode->setCurrentIndex(mode->findData(int(NCompressDialog::NUpdateMode::kUpdate)));
            level->setCurrentIndex(level->findData(0));
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            dialogAccepted = dialog->result() == QDialog::Accepted;
        });
        action(window, "addArchiveAction")->trigger();
        QVERIFY(dialogSeen);
        QVERIFY(sourceSelected);
        QVERIFY(dialogAccepted);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(QDir().mkpath(extracted));
        QVERIFY(runEngine(extracted, {"x", "-y", "--", archive}));
        QFile updated(QDir(extracted).filePath(QStringLiteral("source file.txt")));
        QVERIFY(updated.open(QIODevice::ReadOnly));
        QCOMPARE(updated.readAll(), QByteArray("new bytes"));
        QByteArray listing;
        QVERIFY(runEngine(fixture.path(), {"l", "-slt", "--", archive}, &listing));
        QVERIFY(listing.contains("Method = Store"));
    }

    void addDialogUsesSelectedPathMode_data() {
        QTest::addColumn<int>("pathMode");
        QTest::newRow("full") << int(NWildcard::k_FullPath);
        QTest::newRow("absolute") << int(NWildcard::k_AbsPath);
    }
    void addDialogUsesSelectedPathMode() {
        QFETCH(int, pathMode);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("nested/selected.txt");
        const QString archive = fixture.filePath("paths.zip");
        QVERIFY(QDir().mkpath(QFileInfo(source).absolutePath()));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("full path"), qint64(9));
        file.close();
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        window.openArchive(archive);
        window.show();
        QTRY_COMPARE(table(window)->rowCount(), 1);
        bool accepted = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CreateArchiveDialog *>("createArchiveDialog");
            if (!dialog)
                return;
            auto *directory = dialog->findChild<QLineEdit *>("sourceDirectory");
            auto *available = dialog->findChild<QListWidget *>("availableSources");
            auto *pathModeControl = dialog->findChild<QComboBox *>("archivePathMode");
            if (!directory || !available || !pathModeControl) {
                dialog->reject();
                return;
            }
            directory->setText(QFileInfo(source).absolutePath());
            QMetaObject::invokeMethod(directory, "returnPressed", Qt::DirectConnection);
            for (int row = 0; row < available->count(); ++row)
                if (QFileInfo(available->item(row)->data(Qt::UserRole).toString()).fileName() ==
                    QStringLiteral("selected.txt"))
                    available->item(row)->setSelected(true);
            if (available->selectedItems().size() != 1) {
                dialog->reject();
                return;
            }
            dialog->findChild<QPushButton *>("selectSourcesButton")->click();
            pathModeControl->setCurrentIndex(pathModeControl->findData(pathMode));
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            accepted = dialog->result() == QDialog::Accepted;
        });
        action(window, "addArchiveAction")->trigger();
        QVERIFY(accepted);
        QByteArray listing;
        QVERIFY(runEngine(fixture.path(), {"l", "-slt", "--", archive}, &listing));
        const QString selectedPath = QFileInfo(source).canonicalFilePath();
        QString logical = selectedPath;
        if (pathMode == int(NWildcard::k_FullPath)) {
#ifdef _WIN32
            logical = selectedPath.mid(QDir::rootPath().length());
#else
            logical = selectedPath.mid(1);
#endif
        }
        QVERIFY(listing.contains("Path = " + logical.toUtf8()));
    }
};

QTEST_MAIN(UiUpdateModesTests)
#include "ui_update_modes_tests.moc"
