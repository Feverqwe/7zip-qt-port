#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "external_editor_fixture.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QScopeGuard>

using namespace ExternalEditorFixture;

class ChainBackend final : public ArchiveBackend {
  public:
    int Updates = 0;
    int FailAt = 0;
    int CancelAt = 0;
    QStringList UpdatePaths;
    QStringList OpenPaths;
    void openFolder(const QString &archive, const QString &format,
                    const QString &password = {}) override {
        OpenPaths.append(archive);
        ArchiveBackend::openFolder(archive, format, password);
    }
    ChainBackend() {
        connect(this, &ArchiveBackend::engineStarted, this, [&] {
            if (Updates && Updates == CancelAt)
                cancel();
        });
    }
    void updateSourceFile(const QString &archive, const QString &entry, const QString &root,
                          const QString &output, const QString &format,
                          const QString &password) override {
        ++Updates;
        UpdatePaths.append(archive);
        if (Updates == FailAt)
            emit finished(false, QStringLiteral("Controlled nested chain update failure."));
        else
            ArchiveBackend::updateSourceFile(archive, entry, root, output, format, password);
    }
};

class NestedEditChainTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void nativeEditorUpdatesParentChain_data() {
        QTest::addColumn<int>("outcome");
        QTest::newRow("success") << 0;
        QTest::newRow("inner-write-failure") << 1;
        QTest::newRow("outer-write-failure") << 2;
        QTest::newRow("inner-write-cancel") << 3;
        QTest::newRow("outer-write-cancel") << 4;
        QTest::newRow("parent-prompt-no") << 5;
        QTest::newRow("parent-prompt-escape") << 6;
        QTest::newRow("protected-inner-and-outer") << 7;
    }
    void nativeEditorUpdatesParentChain() {
        QFETCH(int, outcome);
        QTemporaryDir fixture;
        const QString source = fixture.filePath("source");
        QVERIFY(QDir().mkpath(source));
        const QString name = QStringLiteral("изменить файл.txt");
        const QString innerName = QStringLiteral("внутренний архив.7z");
        const QString inner = fixture.filePath(innerName);
        const QString outer = fixture.filePath("внешний архив.zip");
        const QString innerPassword = outcome == 7 ? QStringLiteral("inner 密碼 space") : QString();
        const QString outerPassword =
            outcome == 7 ? QStringLiteral("outer password space") : QString();
        QVERIFY(write(QDir(source).filePath(name), "original"));
        QVERIFY(createArchive(source, inner, name, "7z", innerPassword));
        QVERIFY(createArchive(fixture.path(), outer, innerName, "zip", outerPassword));
        const QByteArray originalOuter = read(outer);
        const QString control = fixture.filePath("editor control");
        const auto stop = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ChainBackend backend;
        backend.FailAt = outcome == 1 ? 1 : (outcome == 2 ? 2 : 0);
        backend.CancelAt = outcome == 3 ? 1 : (outcome == 4 ? 2 : 0);
        MainWindow window(&backend);
        window.resize(outcome == 6 ? QSize(640, 400) : QSize(980, 620));
        window.show();
        bool editPrompt = false, parentPrompt = false, error = false, promptGeometry = true;
        int passwords = 0;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *modal = QApplication::activeModalWidget();
            if (auto *password = dynamic_cast<CPasswordDialog *>(modal)) {
                ++passwords;
                password->findChild<QLineEdit *>("passwordEdit")
                    ->setText(passwords == 1 ? outerPassword : innerPassword);
                password->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                if (box->objectName() == "externalUpdatePrompt") {
                    editPrompt = true;
                    box->button(QMessageBox::Yes)->click();
                } else if (box->objectName() == "modifiedArchiveQuestion") {
                    parentPrompt = true;
                    promptGeometry &= UiInspection::writeReport(
                        *box, outcome == 6 ? "nested-parent-prompt-compact"
                                           : "nested-parent-prompt-normal");
                    promptGeometry &= box->defaultButton() == box->button(QMessageBox::Yes);
                    promptGeometry &= box->escapeButton() == box->button(QMessageBox::Cancel);
                    for (auto *button : box->buttons())
                        promptGeometry &= UiInspection::fullyVisible(*button, *box);
                    for (auto *label : box->findChildren<QLabel *>())
                        if (label->isVisible() && !label->text().isEmpty())
                            promptGeometry &= UiInspection::fullyVisible(*label, *box) &&
                                              UiInspection::labelFits(*label);
                    if (outcome == 6) {
                        responder.stop();
                        QTest::keyClick(box, Qt::Key_Escape);
                    } else
                        box->button(outcome == 5 ? QMessageBox::No : QMessageBox::Yes)->click();
                } else {
                    error = true;
                    box->accept();
                }
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text().contains(QStringLiteral("Закрыть")))
                    close->click();
            }
        });
        responder.start(1);
        window.openArchive(outer);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        action(window, "openInsideAction")->trigger();
        QTRY_COMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                     name);
        table(window)->selectRow(0);
        const QString address = window.findChild<QLineEdit *>("archivePath")->text();
        QVERIFY(address.normalized(QString::NormalizationForm_C).contains(innerName));
        QVERIFY(action(window, "editFileAction")->isEnabled());
        action(window, "editFileAction")->trigger();
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(editPrompt);
        const bool innerFailed = outcome == 1 || outcome == 3;
        if (innerFailed) {
            QTRY_VERIFY(error);
            QCOMPARE(read(temporary), modified);
        } else
            QTRY_VERIFY(!QFileInfo(temporary).exists());
        QCOMPARE(read(outer), originalOuter);
        action(window, "upAction")->trigger();
        QTRY_COMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                     innerName);
        QCOMPARE(parentPrompt, !innerFailed);
        QVERIFY(promptGeometry);
        QCOMPARE(table(window)->currentRow(), 0);
        QVERIFY(window.findChild<QLineEdit *>("archivePath")
                    ->text()
                    .endsWith(QFileInfo(outer).fileName()));
        if (outcome == 0 || outcome == 6) {
            QVERIFY(UiInspection::fullyVisible(*table(window), window));
            QVERIFY(UiInspection::writeReport(window, outcome == 6 ? "nested-parent-owner-compact"
                                                                   : "nested-parent-owner-normal"));
        }
        const bool outerFailed = outcome == 2 || outcome == 4;
        QCOMPARE(backend.Updates, innerFailed || outcome == 5 || outcome == 6 ? 1 : 2);
        if (outcome == 0 || outcome == 7) {
            const QString updatedInner = fixture.filePath("updated.7z");
            QVERIFY(write(updatedInner, contents(outer, innerName, outerPassword)));
            QCOMPARE(contents(updatedInner, name, innerPassword), modified);
            QCOMPARE(error, false);
        } else
            QCOMPARE(read(outer), originalOuter);
        if (outerFailed)
            QVERIFY(error);
        const QString materializedInner = backend.UpdatePaths.front();
        if (outerFailed) {
            QVERIFY(QFileInfo(materializedInner).isFile());
            QCOMPARE(contents(materializedInner, name, innerPassword), modified);
            QVERIFY(QDir(QFileInfo(materializedInner).absolutePath()).removeRecursively());
        } else
            QVERIFY(!QFileInfo(materializedInner).exists());
        if (innerFailed)
            QVERIFY(QDir(QFileInfo(temporary).absolutePath()).removeRecursively());
        responder.stop();
    }
    void readOnlyLevelPreventsChainWrites_data() {
        QTest::addColumn<bool>("parentReadOnly");
        QTest::newRow("readonly-parent-tar") << true;
        QTest::newRow("readonly-child-tar") << false;
    }
    void readOnlyLevelPreventsChainWrites() {
        QFETCH(bool, parentReadOnly);
        QTemporaryDir fixture;
        const QString source = fixture.filePath("source");
        QVERIFY(QDir().mkpath(source));
        const QString name = QStringLiteral("read only файл.txt");
        const QString innerFormat = parentReadOnly ? "7z" : "tar";
        const QString outerFormat = parentReadOnly ? "tar" : "zip";
        const QString innerName = "inner." + innerFormat;
        const QString inner = fixture.filePath(innerName);
        const QString outer = fixture.filePath("outer." + outerFormat);
        QVERIFY(write(QDir(source).filePath(name), "original"));
        QVERIFY(createArchive(source, inner, name, innerFormat));
        QVERIFY(createArchive(fixture.path(), outer, innerName, outerFormat));
        const QByteArray originalOuter = read(outer);
        const QString control = fixture.filePath("editor");
        const auto stop = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ChainBackend backend;
        MainWindow window(&backend);
        window.show();
        bool error = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                error = box->objectName() == "externalUpdateError";
                box->accept();
            }
        });
        responder.start(1);
        window.openArchive(outer);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        action(window, "openInsideAction")->trigger();
        QTRY_COMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                     name);
        const QString materialized = backend.OpenPaths.back();
        QVERIFY(QFileInfo(materialized).isFile());
        table(window)->selectRow(0);
        for (const char *id : {"addArchiveAction", "deleteAction", "renameAction",
                               "createFolderAction", "moveAction", "builtinTextEditAction"})
            QVERIFY(!action(window, id)->isEnabled());
        QVERIFY(action(window, "editFileAction")->isEnabled());
        action(window, "editFileAction")->trigger();
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(error);
        QCOMPARE(read(temporary), modified);
        QCOMPARE(backend.Updates, 0);
        action(window, "upAction")->trigger();
        QVERIFY(!QFileInfo(materialized).exists());
        QCOMPARE(read(outer), originalOuter);
        QVERIFY(QDir(QFileInfo(temporary).absolutePath()).removeRecursively());
        responder.stop();
    }
    void rejectedInnerPasswordRestoresParent_data() {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("wrong-password") << false;
        QTest::newRow("password-cancel") << true;
    }
    void rejectedInnerPasswordRestoresParent() {
        QFETCH(bool, cancel);
        QTemporaryDir fixture;
        const QString innerName = QStringLiteral("protected inner.7z");
        const QString inner = fixture.filePath(innerName);
        const QString outer = fixture.filePath("outer.zip");
        QVERIFY(write(fixture.filePath("file.txt"), "original"));
        QVERIFY(createArchive(fixture.path(), inner, "file.txt", "7z", "dummy 密碼"));
        QVERIFY(createArchive(fixture.path(), outer, innerName));
        const QByteArray original = read(outer);
        ChainBackend backend;
        MainWindow window(&backend);
        window.show();
        bool prompted = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            auto *modal = QApplication::activeModalWidget();
            if (auto *dialog = dynamic_cast<CPasswordDialog *>(modal)) {
                prompted = true;
                if (cancel)
                    dialog->reject();
                else {
                    dialog->findChild<QLineEdit *>("passwordEdit")
                        ->setText("wrong fixture password");
                    dialog->accept();
                }
            } else if (auto *box = qobject_cast<QMessageBox *>(modal))
                box->accept();
            else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text().contains(QStringLiteral("Закрыть")))
                    close->click();
            }
        });
        responder.start(1);
        window.openArchive(outer);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        action(window, "openInsideAction")->trigger();
        QVERIFY(prompted);
        QCOMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                 innerName);
        QVERIFY(window.findChild<QLineEdit *>("archivePath")->text().endsWith("outer.zip"));
        QVERIFY(backend.OpenPaths.size() > 1);
        QVERIFY(!QFileInfo(backend.OpenPaths.back()).exists());
        QCOMPARE(backend.Updates, 0);
        QCOMPARE(read(outer), original);
        responder.stop();
    }
    void nestedSourceMutationsPublishThroughParent() {
        QTemporaryDir fixture;
        const QString source = fixture.filePath("source");
        QVERIFY(QDir().mkpath(source));
        const QString innerName = QStringLiteral("inner.7z");
        const QString inner = fixture.filePath(innerName);
        const QString outer = fixture.filePath("outer.zip");
        QVERIFY(write(QDir(source).filePath("rename.txt"), "keep bytes"));
        QVERIFY(write(QDir(source).filePath("delete.txt"), "remove bytes"));
        QVERIFY(createArchive(source, inner, ".", "7z"));
        QVERIFY(createArchive(fixture.path(), outer, innerName));
        const QByteArray original = read(outer);
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        bool unexpectedError = false, parentPrompt = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                unexpectedError |=
                    box->icon() == QMessageBox::Critical || box->icon() == QMessageBox::Warning;
                parentPrompt |= box->objectName() == "modifiedArchiveQuestion";
                if (auto *yes = box->button(QMessageBox::Yes))
                    yes->click();
                else
                    box->accept();
            }
        });
        responder.start(1);
        window.openArchive(outer);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        action(window, "openInsideAction")->trigger();
        QTRY_COMPARE(table(window)->rowCount(), 2);
        table(window)->selectRow(filesystemRow(window, "rename.txt"));
        window.activateWindow();
        QApplication::processEvents();
        table(window)->setFocus();
        QTRY_VERIFY(table(window)->hasFocus());
        action(window, "renameAction")->trigger();
        auto *editor = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(editor);
        editor->setText(QStringLiteral("renamed файл.txt"));
        QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(filesystemRow(window, QStringLiteral("renamed файл.txt")) >= 0);
        table(window)->selectRow(filesystemRow(window, "delete.txt"));
        action(window, "deleteAction")->trigger();
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<QDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")->setCurrentText("new folder");
            dialog->accept();
        });
        action(window, "createFolderAction")->trigger();
        QTRY_COMPARE(table(window)->rowCount(), 2);
        QCOMPARE(read(outer), original);
        action(window, "upAction")->trigger();
        QVERIFY(parentPrompt);
        QVERIFY(!unexpectedError);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        const QString updated = fixture.filePath("updated.7z");
        QVERIFY(write(updated, contents(outer, innerName)));
        QCOMPARE(contents(updated, QStringLiteral("renamed файл.txt")), QByteArray("keep bytes"));
        QProcess listing;
        listing.start(executable(), {"l", "-slt", "--", updated});
        QVERIFY(listing.waitForFinished(10000));
        QCOMPARE(listing.exitCode(), 0);
        const QByteArray metadata = listing.readAllStandardOutput();
        QVERIFY(metadata.contains("Path = new folder"));
        QVERIFY(!metadata.contains("Path = delete.txt"));
        responder.stop();
    }
};
int main(int argc, char **argv) {
    const int fixture = ExternalEditorFixture::run(argc, argv);
    if (fixture >= 0)
        return fixture;
    QApplication app(argc, argv);
    NestedEditChainTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "nested_edit_chain_tests.moc"
