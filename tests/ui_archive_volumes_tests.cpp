#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "window_state_support.h"
#include <QTreeWidget>

namespace {
bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
const QString kName = QStringLiteral("密碼 с пробелом.bin");
class CDialogAnswers {
    QTimer timer_;

  public:
    QString Password;
    int PasswordPrompts = 0;
    int Errors = 0;
    CDialogAnswers() {
        QObject::connect(&timer_, &QTimer::timeout, &timer_, [this] {
            if (QWidget *widget = QApplication::activeModalWidget()) {
                if (auto *password = dynamic_cast<CPasswordDialog *>(widget)) {
                    ++PasswordPrompts;
                    password->findChild<QLineEdit *>("passwordEdit")->setText(Password);
                    password->accept();
                } else if (auto *box = qobject_cast<QMessageBox *>(widget)) {
                    if (box->icon() == QMessageBox::Critical || box->icon() == QMessageBox::Warning)
                        ++Errors;
                    box->accept();
                } else if (widget->objectName() == "progressDialog") {
                    auto *messages = widget->findChild<QTreeWidget *>("IDL_PROGRESS_MESSAGES");
                    if (messages && messages->isVisible() && messages->topLevelItemCount() > 0)
                        ++Errors;
                    auto *close = widget->findChild<QPushButton *>("IDCANCEL");
                    if (close && close->text().contains(QStringLiteral("Close")))
                        QTimer::singleShot(20, close, &QPushButton::click);
                }
            }
        });
        timer_.start(1);
    }
};
} // namespace

class UiArchiveVolumesTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    bool create(ArchiveBackend &backend, const QString &directory, const QString &format,
                const QString &password, QString &first) {
        const QString source = QDir(directory).filePath(kName);
        const QString other = QDir(directory).filePath("keep.txt");
        if (!writeFile(source, QByteArray(400000, 'x')))
            return false;
        if (!writeFile(other, "kept"))
            return false;
        const QString target = QDir(directory).filePath("volumes." + format);
        CompressionOptions options;
        options.level = 0;
        options.VolumeSizes.Add(128 << 10);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(target, {source, other}, password, !password.isEmpty() && format == "7z",
                       options);
        if (finished.isEmpty() && !finished.wait(30000))
            return false;
        first = target + ".001";
        return finished.last().first().toBool() && QFileInfo::exists(target + ".003");
    }

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void sourceConsumersRoundTrip_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("password");
        QTest::newRow("zip") << QString("zip") << QString();
        QTest::newRow("7z") << QString("7z") << QString();
        QTest::newRow("zip-password") << QString("zip") << QString("fixture123");
        QTest::newRow("7z-header-password") << QString("7z") << QString("fixture123");
    }
    void sourceConsumersRoundTrip() {
        QFETCH(QString, format);
        QFETCH(QString, password);
        QTemporaryDir fixture, output;
        QVERIFY(fixture.isValid());
        QVERIFY(output.isValid());
        ArchiveBackend backend;
        QString first;
        QVERIFY(create(backend, fixture.path(), format, password, first));
        const QByteArray firstBytes = readFile(first);
        CDialogAnswers answers;
        answers.Password = password;
        MainWindow window(&backend);
        window.show();
        window.openArchive(first);
        QTRY_COMPARE(table(window)->rowCount(), 2);
        const int selected = rowWithPath(window, kName);
        QVERIFY(selected >= 0);
        table(window)->selectRow(selected);
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        QSignalSpy completed(&backend, &ArchiveBackend::finished);
        window.testArchive();
        QCOMPARE(started.size(), password.isEmpty() ? 1 : 2);
        QVERIFY(!backend.busy());
        QVERIFY(completed.last().first().toBool());
        QCOMPARE(answers.Errors, 0);
        QCOMPARE(QDir(output.path()).entryList(QDir::Files), QStringList());
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        window.extractTo(output.path(), false, password, options);
        QCOMPARE(readFile(output.filePath(kName)), QByteArray(400000, 'x'));
        QCOMPARE(readFile(output.filePath("keep.txt")), QByteArray("kept"));
        QCOMPARE(readFile(first), firstBytes);
        QVERIFY(!backend.busy());
        QCOMPARE(answers.Errors, 0);
        if (format == "7z" && !password.isEmpty())
            QVERIFY(answers.PasswordPrompts > 0);
        if (!password.isEmpty()) {
            const QString target = output.filePath(kName);
            QVERIFY(writeFile(target, "preserved"));
            answers.Password = "wrong-password";
            completed.clear();
            window.extractTo(output.path(), false, answers.Password, options);
            QCOMPARE(readFile(target), QByteArray("preserved"));
            QVERIFY(!completed.isEmpty());
            QVERIFY(!completed.last().first().toBool());
            QVERIFY(answers.Errors > 0);
            QVERIFY(!backend.busy());
        }
    }
    void missingCorruptAndCancelledConsumers_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("missing-open") << 0;
        QTest::newRow("corrupt-test") << 1;
        QTest::newRow("cancel-test") << 2;
        QTest::newRow("cancel-extract") << 3;
        QTest::newRow("cancel-open") << 4;
    }
    void missingCorruptAndCancelledConsumers() {
        QFETCH(int, mode);
        QTemporaryDir fixture, output;
        QVERIFY(fixture.isValid());
        QVERIFY(output.isValid());
        ArchiveBackend backend;
        QString first;
        QVERIFY(create(backend, fixture.path(), "7z", {}, first));
        const QString second = first.left(first.size() - 3) + "002";
        CDialogAnswers answers;
        MainWindow window(&backend);
        window.show();
        if (mode == 4) {
            connect(&backend, &ArchiveBackend::engineStarted, &backend, &ArchiveBackend::cancel);
            window.openArchive(first);
            QCOMPARE(backend.lastResult(), E_ABORT);
            QVERIFY(!backend.busy());
            QCOMPARE(answers.Errors, 0);
            QVERIFY(action(window, "toolbarAddAction")->isEnabled());
            return;
        }
        if (mode == 0) {
            QVERIFY(QFile::remove(second));
            window.openArchive(first);
            QVERIFY(answers.Errors > 0);
            QVERIFY(!backend.busy());
            QVERIFY(action(window, "toolbarAddAction")->isEnabled());
            return;
        }
        window.openArchive(first);
        QTRY_COMPARE(table(window)->rowCount(), 2);
        const QByteArray firstBytes = readFile(first);
        if (mode == 1)
            QVERIFY(writeFile(second, QByteArray(readFile(second).size(), 'z')));
        int stages = 0;
        if (mode >= 2)
            connect(&backend, &ArchiveBackend::engineStarted, &backend, [&] {
                if (++stages >= (mode == 3 ? 2 : 1))
                    backend.cancel();
            });
        QSignalSpy completed(&backend, &ArchiveBackend::finished);
        if (mode < 3)
            window.testArchive();
        else {
            ExtractionOptions options;
            options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
            window.extractTo(output.path(), false, {}, options);
        }
        QVERIFY(!backend.busy());
        QVERIFY(!completed.isEmpty());
        QVERIFY(!completed.last().first().toBool());
        QCOMPARE(readFile(first), firstBytes);
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        if (mode == 1)
            QVERIFY(answers.Errors > 0);
        else
            QCOMPARE(backend.lastResult(), E_ABORT);
    }
};
QTEST_MAIN(UiArchiveVolumesTests)
#include "ui_archive_volumes_tests.moc"
