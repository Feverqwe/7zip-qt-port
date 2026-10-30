#include "7zip/UI/FileManager/resource.h"
#include "archive/codecs/archivecodeccatalog.h"
#include "dialogs/combo/ComboDialog.h"
#include "filemanager/support/window_state_support.h"
#include "localization/LangUtils.h"
#include "ui/support/layout_inspection.h"
#include <QScopeGuard>
#include <QStandardPaths>

namespace {
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
} // namespace

class NativeCommentUiTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;
    bool run(const QString &directory, const QStringList &arguments) {
        QProcess process;
        process.setWorkingDirectory(directory);
        process.start(engine_, arguments);
        return process.waitForFinished(10000) && process.exitCode() == 0;
    }
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; actual comment command UX unverified");
#else
        engine_ = QStandardPaths::findExecutable("7z");
        QVERIFY(!engine_.isEmpty());
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
    void cleanup() {
        SaveRegLang(UString(L"-"), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
    void actualCommandWritesCancelsAndReportsError_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("encrypted");
        QTest::addColumn<QString>("outcome");
        for (const QString &language : {"-", "ru"})
            for (bool encrypted : {false, true})
                for (const QString &outcome : {"success", "cancel", "failure"})
                    QTest::newRow(qPrintable(language + QString::number(encrypted) + outcome))
                        << language << encrypted << outcome;
    }
    void actualCommandWritesCancelsAndReportsError() {
        QFETCH(QString, language);
        QFETCH(bool, encrypted);
        QFETCH(QString, outcome);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("имя 空 with spaces.bin");
        const QByteArray payload("binary\0payload\xff", 15);
        QVERIFY(writeFile(fixture.filePath(name), payload));
        QVERIFY(writeFile(fixture.filePath("other.txt"), "other data"));
        const QString archive = fixture.filePath("archive.unknown");
        QStringList args{"a", "-tzip"};
        if (encrypted)
            args << "-plocal fixture password" << "-mem=AES256";
        args << "--" << archive << name << "other.txt";
        QVERIFY(run(fixture.path(), args));
        const QByteArray original = readFile(archive);
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 2);
        const int row = rowWithPath(window, name);
        QVERIFY(row >= 0);
        table(window)->setCurrentCell(row, 0);
        table(window)->selectAll(); // Source comment acts on focus, not the marked list.
        QTRY_VERIFY(action(window, "commentAction")->isEnabled());
        bool observed = false;
        QStringList errors;
        QTimer responder;
        responder.setInterval(2);
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *widget = QApplication::activeModalWidget();
            if (auto *dialog = dynamic_cast<CComboDialog *>(widget)) {
                if (observed)
                    return;
                observed = true;
                const auto dismiss = qScopeGuard([&] {
                    if (dialog->isVisible())
                        dialog->reject();
                });
                QVERIFY2(dialog->windowTitle().contains(name), qPrintable(dialog->windowTitle()));
                QCOMPARE(dialog->findChild<QLabel *>("comboPrompt")->text(),
                         QString::fromWCharArray(LangString(IDS_COMMENT2)));
                auto *edit = dialog->findChild<QComboBox *>("comboValue");
                QVERIFY(edit && edit->isEditable());
                QVERIFY(edit->currentText().isEmpty());
                if (!encrypted && outcome == "success") {
                    for (int width : {360, 260}) {
                        dialog->resize(width, 150);
                        QApplication::processEvents();
                        QVERIFY(UiInspection::fullyVisible(*edit, *dialog));
                        auto *buttons = dialog->findChild<QDialogButtonBox *>("comboButtons");
                        QVERIFY(UiInspection::fullyVisible(*buttons->button(QDialogButtonBox::Ok),
                                                           *dialog));
                        QVERIFY(UiInspection::fullyVisible(
                            *buttons->button(QDialogButtonBox::Cancel), *dialog));
                        QVERIFY(UiInspection::writeReport(*dialog,
                                                          QString("comment-%1-%2")
                                                              .arg(language == "ru" ? "ru" : "en")
                                                              .arg(width)));
                    }
                }
                edit->setEditText(outcome == "failure" ? QString(30000, QChar(0x7A7A))
                                                       : QStringLiteral("  Комментарий 空  "));
                if (outcome == "cancel")
                    dialog->reject();
                else
                    dialog->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(widget)) {
                const auto close = qScopeGuard([box] { box->accept(); });
                if (box->icon() == QMessageBox::Critical)
                    errors.append(box->text());
                if (box->objectName() == "panelArchiveError") {
#ifdef Q_OS_MACOS
                    QCOMPARE(box->text(),
                             QString::fromUtf8(language == "ru" ? "Ошибка записи комментария"
                                                                : "Set Comment Error"));
                    QVERIFY(!box->informativeText().isEmpty());
#else
                    QCOMPARE(box->windowTitle(), QString::fromUtf8(language == "ru" ? "Ошибка записи комментария" : "Set Comment Error"));
                    QVERIFY(!box->text().isEmpty());
#endif
                }
            }
        });
        responder.start();
        action(window, "commentAction")->trigger();
        QVERIFY(observed);
        QCOMPARE(errors.size(), outcome == "failure" ? 1 : 0);
        QCOMPARE(table(window)->currentRow(), rowWithPath(window, name));
        QTRY_VERIFY(action(window, "commentAction")->isEnabled());
        QVERIFY(!backend.busy());
        if (outcome != "success") {
            QCOMPARE(readFile(archive), original);
            return;
        }
        QVERIFY(readFile(archive) != original);
        // Opening the editor again proves native properties/reopen/refresh and
        // raw whitespace survive the actual command, rather than a test DTO.
        bool reopened = false;
        disconnect(&responder, nullptr, this, nullptr);
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *dialog = dynamic_cast<CComboDialog *>(QApplication::activeModalWidget());
            if (!dialog || reopened)
                return;
            reopened = true;
            QCOMPARE(dialog->findChild<QComboBox *>("comboValue")->currentText(),
                     QStringLiteral("  Комментарий 空  "));
            dialog->reject();
        });
        action(window, "commentAction")->trigger();
        QVERIFY(reopened);
        const QString extracted = fixture.filePath("extracted");
        QVERIFY(run(fixture.path(),
                    {"x", "-tzip", "-plocal fixture password", "-o" + extracted, "--", archive}));
        QCOMPARE(readFile(extracted + "/" + name), payload);
        QCOMPARE(readFile(extracted + "/other.txt"), QByteArray("other data"));
    }
    void nonZipCommandRemainsDisabled() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(writeFile(fixture.filePath("payload.bin"), "data"));
        const QString archive = fixture.filePath("archive.7z");
        QVERIFY(run(fixture.path(), {"a", "-t7z", "--", archive, "payload.bin"}));
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QVERIFY(!action(window, "commentAction")->isEnabled());
    }
    void nestedCommentPublishesOnlyAfterParentConfirmation_data() {
        QTest::addColumn<bool>("publish");
        QTest::newRow("publish") << true;
        QTest::newRow("discard") << false;
    }
    void nestedCommentPublishesOnlyAfterParentConfirmation() {
        QFETCH(bool, publish);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("имя 空.txt");
        const QString innerName = QStringLiteral("inner 空 with spaces.zip");
        const QString inner = fixture.filePath(innerName);
        const QString outer = fixture.filePath("outer.zip");
        QVERIFY(writeFile(fixture.filePath(name), "original bytes"));
        QVERIFY(run(fixture.path(), {"a", "-tzip", "--", inner, name}));
        QVERIFY(run(fixture.path(), {"a", "-tzip", "--", outer, innerName}));
        const QByteArray original = readFile(outer);
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        bool commentShown = false, parentShown = false;
        QStringList errors;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *modal = QApplication::activeModalWidget();
            if (auto *dialog = dynamic_cast<CComboDialog *>(modal)) {
                commentShown = true;
                dialog->findChild<QComboBox *>("comboValue")->setEditText("nested comment");
                dialog->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                if (box->objectName() == "modifiedArchiveQuestion") {
                    parentShown = true;
                    box->button(publish ? QMessageBox::Yes : QMessageBox::No)->click();
                } else {
                    errors.append(box->text());
                    box->accept();
                }
            }
        });
        responder.start(2);
        window.openArchive(outer);
        QTRY_VERIFY(rowWithPath(window, innerName) >= 0);
        table(window)->selectRow(rowWithPath(window, innerName));
        action(window, "openInsideAction")->trigger();
        QTRY_VERIFY(rowWithPath(window, name) >= 0);
        table(window)->selectRow(rowWithPath(window, name));
        QVERIFY(action(window, "commentAction")->isEnabled());
        action(window, "commentAction")->trigger();
        QVERIFY(commentShown);
        QVERIFY(errors.isEmpty());
        QCOMPARE(readFile(outer), original);
        action(window, "upAction")->trigger();
        QTRY_VERIFY(rowWithPath(window, innerName) >= 0);
        QVERIFY(parentShown);
        QVERIFY(errors.isEmpty());
        if (!publish) {
            QCOMPARE(readFile(outer), original);
            return;
        }
        QVERIFY(readFile(outer) != original);
        const QString extracted = fixture.filePath("extract");
        QVERIFY(run(fixture.path(), {"x", "-o" + extracted, "--", outer}));
        ArchiveBackend check(nullptr, engine_);
        QCOMPARE(check.loadCodecCatalog(module_, version_), S_OK);
        MainWindow reopened(&check);
        reopened.show();
        reopened.openArchive(extracted + "/" + innerName);
        QTRY_VERIFY(rowWithPath(reopened, name) >= 0);
        table(reopened)->selectRow(rowWithPath(reopened, name));
        bool observed = false;
        disconnect(&responder, nullptr, this, nullptr);
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *dialog = dynamic_cast<CComboDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            const auto close = qScopeGuard([&] { dialog->reject(); });
            observed = true;
            QCOMPARE(dialog->findChild<QComboBox *>("comboValue")->currentText(),
                     QStringLiteral("nested comment"));
        });
        action(reopened, "commentAction")->trigger();
        QVERIFY(observed);
        QVERIFY(run(extracted, {"x", "-ounpacked", "--", innerName}));
        QCOMPARE(readFile(extracted + "/unpacked/" + name), QByteArray("original bytes"));
    }
};
QTEST_MAIN(NativeCommentUiTests)
#include "native_comment_ui_tests.moc"
