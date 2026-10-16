#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
#include "fixtures/widget_dialogs.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QCheckBox>
#include <QScopeGuard>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTreeWidget>

namespace {
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
class CMessageAnswers {
    WidgetDialogFixture &dialogs_;
    QTimer timer_;
    bool cancelQuestions_;

  public:
    QStringList Errors;
    bool CancelledQuestion = false;
    explicit CMessageAnswers(WidgetDialogFixture &dialogs, bool cancelQuestions = false)
        : dialogs_(dialogs), cancelQuestions_(cancelQuestions) {
        dialogs_.connectResponder(timer_, &timer_, [this] {
            auto *widget = dialogs_.activeDialog();
            if (auto *box = qobject_cast<QMessageBox *>(widget)) {
                if (box->icon() == QMessageBox::Warning || box->icon() == QMessageBox::Critical)
                    Errors.append(box->text());
                if (box->icon() == QMessageBox::Question && box->button(QMessageBox::Yes)) {
                    if (cancelQuestions_ && box->button(QMessageBox::Cancel)) {
                        CancelledQuestion = true;
                        box->button(QMessageBox::Cancel)->click();
                    } else
                        box->button(QMessageBox::Yes)->click();
                } else
                    box->accept();
            } else if (widget && widget->objectName() == "progressDialog") {
                auto *close = widget->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text().contains("Close"))
                    close->click();
            }
        });
        timer_.start(2);
    }
};
// Stop at the first visible native progress event and record the source phase.
// A small native update can already have entered publication by this event.
class CNativeCancel final : public QObject {
    bool &observed_;
    bool &publication_;
    const QString &archive_;
    const QByteArray &before_;

  public:
    CNativeCancel(bool &observed, bool &publication, const QString &archive,
                  const QByteArray &before)
        : observed_(observed), publication_(publication), archive_(archive), before_(before) {
        qApp->installEventFilter(this);
    }
    ~CNativeCancel() override { qApp->removeEventFilter(this); }

  protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::Show && !observed_)
            if (auto *progress = dynamic_cast<CProgressDialog *>(object)) {
                observed_ = true;
                progress->Sync.Set_Stopped(true);
                {
                    NWindows::NSynchronization::CCriticalSectionLock lock(progress->Sync._cs);
                    publication_ = QDir::cleanPath(QDir::fromNativeSeparators(ToQString(
                                       progress->Sync._filePath))) == QDir::cleanPath(archive_);
                }
                // MoveArc_Start names the destination before its stop check.
                // After Finish clears that name, the committed bytes identify
                // the later reopen phase instead.
                publication_ = publication_ || readFile(archive_) != before_;
            }
        return false;
    }
};
} // namespace

class CodecCommandsUiTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture dialogs_;
    QString engine_, module_;
    quint32 version_ = 0;
    bool run(const QString &directory, const QStringList &arguments) {
        QProcess engine;
        engine.setWorkingDirectory(directory);
        engine.start(engine_, arguments);
        return engine.waitForFinished(10000) && engine.exitCode() == 0;
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native command UX remains unverified");
#else
        engine_ = QStandardPaths::findExecutable("7zz");
        if (engine_.isEmpty())
            QSKIP("Official engine unavailable; native command UX remains unverified");
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        version_ = backend.codecCatalog()->version();
#endif
    }
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void actualCopyCommandUsesDetectedWriter_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("cancel");
        for (const QString &format : {"7z", "zip", "gzip", "bzip2", "xz", "tar", "wim"})
            for (bool cancel : {false, true})
                QTest::newRow(qPrintable(format + (cancel ? " cancel" : " success")))
                    << format << cancel;
    }
    void actualCopyCommandUsesDetectedWriter() {
        QFETCH(QString, format);
        QFETCH(bool, cancel);
        QTemporaryDir fixture(QDir::tempPath() + "/codec UI 空-XXXXXX"), output;
        QVERIFY(fixture.isValid() && output.isValid());
        QString name = QStringLiteral("entry 空 with spaces.bin");
#ifdef Q_OS_WIN
        // Source gzip names are limited by CP_ACP; keep the containing path Unicode.
        if (format == "gzip")
            name = QStringLiteral("entry with spaces.bin");
#endif
        const QString source = fixture.filePath(name);
        const QString archive = fixture.filePath(name + ".unknown");
        QVERIFY(writeFile(source, "original"));
        QVERIFY(run(fixture.path(), {"a", "-t" + format, "--", archive, name}));
        const QByteArray before = readFile(archive);
        QVERIFY(!before.isEmpty());
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMessageAnswers messages(dialogs_);
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QVERIFY(action(window, "deleteAction")->isEnabled());
        QVERIFY(action(window, "renameAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        const QByteArray replacement = format == "wim" && cancel
                                           ? QByteArray(1024 * 1024, 'x')
                                           : QByteArray("updated\0binary bytes", 20);
        QVERIFY(writeFile(source, replacement));
        if (!action(window, "twoPanelsAction")->isChecked())
            action(window, "twoPanelsAction")->trigger();
        auto *left = window.findChild<QWidget *>("filePanel0");
        auto *leftAddress = left->findChild<QLineEdit *>("archivePath");
        auto *leftTable = left->findChild<QTableWidget *>("archiveTable");
        leftAddress->setText(fixture.path());
        emit leftAddress->returnPressed();
        QTRY_VERIFY(leftTable->isEnabled());
        auto *right = window.findChild<QWidget *>("filePanel1");
        auto *rightTable = right->findChild<QTableWidget *>("archiveTable");
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        rightTable->setFocus();
        QTRY_VERIFY(rightTable->hasFocus());
        window.openArchive(archive);
        QTRY_COMPARE(rightTable->rowCount(), 1);
        QTRY_VERIFY(rightTable->isEnabled());
        QTRY_VERIFY(action(window, "testArchiveAction")->isEnabled());
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QApplication::processEvents();
        QTRY_VERIFY(leftTable->isEnabled());
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        int sourceRow = -1;
        for (int row = 0; row < leftTable->rowCount(); ++row)
            if (leftTable->item(row, 0)->text().normalized(QString::NormalizationForm_C) == name)
                sourceRow = row;
        QVERIFY(sourceRow >= 0);
        leftTable->selectRow(sourceRow);
        bool submitted = false;
        dialogs_.respond(window, [&](QDialog &active) {
            auto *dialog = qobject_cast<CopyDialog *>(&active);
            QVERIFY(dialog);
            const auto closeOnFailure = qScopeGuard([&] {
                if (!submitted)
                    dialog->reject();
            });
            auto *destination = dialog->findChild<QComboBox *>("copyDestination");
            QVERIFY(destination);
            QCOMPARE(QDir::cleanPath(QDir::fromNativeSeparators(destination->currentText())),
                     QDir::cleanPath(archive));
            dialog->resize(cancel ? QSize(480, 320) : QSize(640, 400));
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*destination, *dialog));
            QVERIFY(UiInspection::writeReport(
                *dialog, QString("codec-copy-%1-%2").arg(format).arg(dialog->width())));
            QVERIFY(UiInspection::fullyVisible(
                *dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok), *dialog));
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            submitted = dialog->result() == QDialog::Accepted;
        });
        bool observedStart = false;
        bool publicationAtCancel = false;
        std::unique_ptr<CNativeCancel> nativeCancel;
        if (cancel)
            nativeCancel = std::make_unique<CNativeCancel>(observedStart, publicationAtCancel,
                                                           archive, before);
        const auto connection = connect(&backend, &ArchiveBackend::engineStarted, &window, [&] {
            if (cancel)
                return; // Native writer cancellation belongs to its source Sync.
            if (!observedStart) {
                observedStart = true;
                if (cancel)
                    backend.cancel();
            }
        });
        action(window, "copyAction")->trigger();
        nativeCancel.reset();
        disconnect(connection);
        QVERIFY(submitted && observedStart);
        QVERIFY2(messages.Errors.isEmpty(), qPrintable(messages.Errors.join('\n')));
        QVERIFY(!backend.busy());
        if (cancel)
            qInfo() << "Native cancel phase:" << (publicationAtCancel ? "publication" : "writer");
        if (cancel && (!publicationAtCancel || readFile(archive) == before))
            QCOMPARE(readFile(archive), before);
        else {
            QVERIFY(run(fixture.path(), {"x", "-y", "-o" + output.path(), "--", archive}));
            QCOMPARE(readFile(output.filePath(name)), replacement);
        }
        QCOMPARE(readFile(source), replacement);
        QVERIFY(
            QDir(fixture.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void actualInsideActionsReachNativeParser_data() {
        QTest::addColumn<QByteArray>("command");
        QTest::newRow("one-handler") << QByteArray("openInsideOneAction");
        QTest::newRow("parser") << QByteArray("openInsideParserAction");
    }
    void singleStreamDeleteUsesOriginalHandlerContract_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("cancel");
        for (const QString &format : {"bzip2", "xz"})
            for (bool cancel : {false, true})
                QTest::newRow(qPrintable(format + (cancel ? " cancel" : " handler result")))
                    << format << cancel;
    }
    void singleStreamDeleteUsesOriginalHandlerContract() {
        QFETCH(QString, format);
        QFETCH(bool, cancel);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("stream 空 with spaces.bin");
        const QString source = fixture.filePath(name);
        const QString archive = fixture.filePath(name + ".unknown");
        QVERIFY(writeFile(source, "stream payload"));
        QVERIFY(run(fixture.path(), {"a", "-t" + format, "--", archive, name}));
        const QByteArray before = readFile(archive);
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMessageAnswers messages(dialogs_, cancel);
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QTRY_VERIFY(action(window, "deleteAction")->isEnabled());
        QSignalSpy processes(&backend, &ArchiveBackend::engineStarted);
        action(window, "deleteAction")->trigger();
        QCOMPARE(messages.CancelledQuestion, cancel);
        if (cancel)
            QCOMPARE(processes.size(), 0);
        QCOMPARE(messages.Errors.isEmpty(), cancel || format == "xz");
        if (cancel || format == "bzip2")
            QCOMPARE(readFile(archive), before);
        else {
            // Original XzHandler::UpdateItems encodes an empty XZ stream for
            // zero requested items; GetNumberOfItems still returns one.
            QVERIFY(readFile(archive) != before);
            QTemporaryDir output;
            QVERIFY(output.isValid());
            QVERIFY(run(fixture.path(), {"x", "-y", "-o" + output.path(), "--", archive}));
            QVERIFY(QFileInfo::exists(output.filePath(name)));
            QCOMPARE(QFileInfo(output.filePath(name)).size(), qint64(0));
        }
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!backend.busy());
    }
    void unavailableCatalogKeepsReadingAndDisablesMutation_data() {
        QTest::addColumn<bool>("mismatch");
        QTest::newRow("missing") << false;
        QTest::newRow("version mismatch") << true;
    }
    void unavailableCatalogKeepsReadingAndDisablesMutation() {
        QFETCH(bool, mismatch);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(writeFile(fixture.filePath("payload.bin"), "fallback"));
        const QString archive = fixture.filePath("content.unknown");
        QVERIFY(run(fixture.path(), {"a", "-tzip", "--", archive, "payload.bin"}));
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(mismatch ? module_ : fixture.filePath("missing module"),
                                          mismatch ? version_ + 1 : version_),
                 E_FAIL);
        MainWindow window(&backend);
        CMessageAnswers messages(dialogs_);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QVERIFY(!action(window, "toolbarAddAction")->isEnabled());
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(messages.Errors.isEmpty());
    }
    void actualInsideActionsReachNativeParser() {
        QFETCH(QByteArray, command);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("payload.bin");
        const QString archive = fixture.filePath(QStringLiteral("архив 空.unknown"));
        QVERIFY(writeFile(source, "inside fixture"));
        QVERIFY(run(fixture.path(), {"a", "-tzip", "--", archive, "payload.bin"}));
        QByteArray bytes = readFile(archive);
        if (command == "openInsideParserAction") {
            bytes += bytes;
            QVERIFY(writeFile(archive, bytes));
        }
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMessageAnswers messages(dialogs_);
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *address = window.findChild<QLineEdit *>("archivePath");
        QVERIFY(address);
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_VERIFY(filesystemRow(window, QFileInfo(archive).fileName()) >= 0);
        table(window)->selectRow(filesystemRow(window, QFileInfo(archive).fileName()));
        QTRY_VERIFY(action(window, command.constData())->isEnabled());
        action(window, command.constData())->trigger();
        QTRY_VERIFY(action(window, "testArchiveAction")->isEnabled());
        if (command == "openInsideParserAction") {
            QVERIFY(table(window)->rowCount() >= 2);
            QVERIFY(!action(window, "deleteAction")->isEnabled());
            QVERIFY(!action(window, "toolbarAddAction")->isEnabled());
        } else {
            QCOMPARE(table(window)->rowCount(), 1);
            QCOMPARE(table(window)->item(0, 0)->text(), QString("payload.bin"));
            QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        }
        QVERIFY2(messages.Errors.isEmpty(), qPrintable(messages.Errors.join('\n')));
        QCOMPARE(readFile(archive), bytes);
    }
};
QTEST_MAIN(CodecCommandsUiTests)
#include "codec_commands_ui_tests.moc"
