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
};

QTEST_MAIN(UiUpdateModesTests)
#include "ui_update_modes_tests.moc"
