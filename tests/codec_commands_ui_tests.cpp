#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "createarchivedialog.h"
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
    QTimer timer_;

  public:
    QStringList Errors;
    CMessageAnswers() {
        QObject::connect(&timer_, &QTimer::timeout, &timer_, [this] {
            auto *widget = QApplication::activeModalWidget();
            if (auto *box = qobject_cast<QMessageBox *>(widget)) {
                if (box->icon() == QMessageBox::Warning || box->icon() == QMessageBox::Critical)
                    Errors.append(box->text());
                if (box->icon() == QMessageBox::Question && box->button(QMessageBox::Yes))
                    box->button(QMessageBox::Yes)->click();
                else
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
} // namespace

class CodecCommandsUiTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
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
    void actualAddCommandUsesDetectedWriter_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("cancel");
        for (const QString &format : {"7z", "zip", "gzip", "bzip2", "xz", "tar", "wim"})
            for (bool cancel : {false, true})
                QTest::newRow(qPrintable(format + (cancel ? " cancel" : " success")))
                    << format << cancel;
    }
    void actualAddCommandUsesDetectedWriter() {
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
        CMessageAnswers messages;
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QTRY_VERIFY(action(window, "addArchiveAction")->isEnabled());
        QVERIFY(action(window, "deleteAction")->isEnabled());
        QVERIFY(action(window, "renameAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        const QByteArray replacement("updated\0binary bytes", 20);
        QVERIFY(writeFile(source, replacement));
        bool submitted = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CreateArchiveDialog *>("createArchiveDialog");
            QVERIFY(dialog);
            const auto closeOnFailure = qScopeGuard([&] {
                if (!submitted)
                    dialog->reject();
            });
            auto *types = dialog->findChild<QComboBox *>("archiveFormat");
            QVERIFY(types);
            auto *directory = dialog->findChild<QLineEdit *>("sourceDirectory");
            QVERIFY(directory);
            directory->setText(fixture.path());
            QMetaObject::invokeMethod(directory, "returnPressed", Qt::DirectConnection);
            auto *available = dialog->findChild<QListWidget *>("availableSources");
            QVERIFY(available);
            for (int row = 0; row < available->count(); ++row)
                if (available->item(row)->data(Qt::UserRole).toString() == source)
                    available->item(row)->setSelected(true);
            QCOMPARE(available->selectedItems().size(), 1);
            dialog->findChild<QPushButton *>("selectSourcesButton")->click();
            QCOMPARE(types->currentText(), format);
            auto *method = dialog->findChild<QComboBox *>("compressionMethod");
            QVERIFY(method);
            if (format == "gzip")
                QCOMPARE(method->currentText(), QString("Deflate"));
            else if (format == "bzip2")
                QCOMPARE(method->currentText(), QString("BZip2"));
            else if (format == "xz")
                QCOMPARE(method->currentText(), QString("LZMA2"));
            dialog->resize(cancel ? QSize(640, 480) : QSize(760, 720));
            QApplication::processEvents();
            dialog->findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(types);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*types, *dialog));
            QVERIFY(UiInspection::writeReport(
                *dialog, QString("codec-add-%1-%2").arg(format).arg(dialog->width())));
            QVERIFY(UiInspection::fullyVisible(
                *dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok), *dialog));
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            submitted = dialog->result() == QDialog::Accepted;
        });
        bool observedStart = false;
        QTimer nativeCancel;
        if (format == "wim" && cancel) {
            connect(&nativeCancel, &QTimer::timeout, &window, [&] {
                for (QWidget *widget : QApplication::topLevelWidgets())
                    if (auto *progress = dynamic_cast<CProgressDialog *>(widget)) {
                        observedStart = true;
                        progress->Sync.Set_Stopped(true);
                        nativeCancel.stop();
                        return;
                    }
            });
            nativeCancel.start(1);
        }
        const auto connection = connect(&backend, &ArchiveBackend::engineStarted, &window, [&] {
            if (format == "wim" && cancel)
                return; // Native writer cancellation belongs to its source Sync.
            if (!observedStart) {
                observedStart = true;
                if (cancel)
                    backend.cancel();
            }
        });
        action(window, "addArchiveAction")->trigger();
        nativeCancel.stop();
        disconnect(connection);
        QVERIFY(submitted && observedStart);
        QVERIFY2(messages.Errors.isEmpty(), qPrintable(messages.Errors.join('\n')));
        QVERIFY(!backend.busy());
        if (cancel)
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
        CMessageAnswers messages;
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QTRY_VERIFY(action(window, "deleteAction")->isEnabled());
        bool observed = false;
        const auto connection = connect(&backend, &ArchiveBackend::engineStarted, &window, [&] {
            if (!observed) {
                observed = true;
                if (cancel)
                    backend.cancel();
            }
        });
        action(window, "deleteAction")->trigger();
        disconnect(connection);
        QVERIFY(observed);
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
        CMessageAnswers messages;
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
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
        CMessageAnswers messages;
        MainWindow window(&backend);
        window.show();
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
            QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        } else {
            QCOMPARE(table(window)->rowCount(), 1);
            QCOMPARE(table(window)->item(0, 0)->text(), QString("payload.bin"));
            QVERIFY(action(window, "addArchiveAction")->isEnabled());
        }
        QVERIFY2(messages.Errors.isEmpty(), qPrintable(messages.Errors.join('\n')));
        QCOMPARE(readFile(archive), bytes);
    }
};
QTEST_MAIN(CodecCommandsUiTests)
#include "codec_commands_ui_tests.moc"
