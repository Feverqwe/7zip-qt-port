#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "external_editor_fixture.h"
#include "fixtures/widget_dialogs.h"
#include "layout_inspection.h"
#include "native_property_failure_runtime.h"
#include "window_state_support.h"
#include <QScopeGuard>
#include <QTreeWidget>

using namespace ExternalEditorFixture;

class NestedEditChainTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture widgetDialogs_;
    NativePropertyFailureRuntime runtime_, readOnlyRuntime_;
  private slots:
    void initTestCase() {
        for (auto *runtime : {&runtime_, &readOnlyRuntime_})
            QVERIFY(runtime->prepare(QString::fromUtf8(SEVENZIP_EXTERNAL_RUNTIME),
                                     QString::fromUtf8(SEVENZIP_EXTERNAL_FORMAT_FIXTURE)));
        QVERIFY(runtime_.enableWriter());
    }
    void init() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QVERIFY(runtime_.clear());
        QVERIFY(readOnlyRuntime_.clear());
    }
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
        const QString innerName = QStringLiteral("внутренний архив.") +
                                  (outcome == 1 || outcome == 3 ? "fixture7z" : "7z");
        const QString inner = fixture.filePath(innerName);
        const QString outer = fixture.filePath(
            outcome == 2 || outcome == 4 ? "внешний архив.fixture7z" : "внешний архив.zip");
        const QString innerPassword = outcome == 7 ? QStringLiteral("inner 密碼 space") : QString();
        const QString outerPassword =
            outcome == 7 ? QStringLiteral("outer password space") : QString();
        QVERIFY(write(QDir(source).filePath(name), "original"));
        QVERIFY(createArchive(source, inner, name, "7z", innerPassword));
        QVERIFY(createArchive(fixture.path(), outer, innerName,
                              outcome == 2 || outcome == 4 ? "7z" : "zip", outerPassword));
        const QByteArray originalOuter = read(outer);
        const QString control = fixture.filePath("editor control");
        const auto stop = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ArchiveBackend backend(nullptr, runtime_.engine());
        if (outcome >= 1 && outcome <= 4)
            QVERIFY(runtime_.armWriter(outcome <= 2 ? E_FAIL : E_ABORT, outcome >= 3));
        bool writerCancelled = false;
        MainWindow window(&backend);
        window.resize(outcome == 6 ? QSize(640, 400) : QSize(980, 620));
        window.show();
        bool editPrompt = false, parentPrompt = false, error = false, promptGeometry = true;
        int passwords = 0;
        bool cleaningUp = false;
        QString openingMessage;
        QString editedOutput, nestedSpool;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            auto *modal = widgetDialogs_.activeDialog();
            if (auto *question = qobject_cast<QMessageBox *>(modal);
                question && question->objectName() == "progressCancelQuestion") {
                question->button(QMessageBox::Yes)->click();
            } else if (auto *password = dynamic_cast<CPasswordDialog *>(modal)) {
                ++passwords;
                if (cleaningUp) {
                    password->reject();
                    return;
                }
                password->findChild<QLineEdit *>("passwordEdit")
                    ->setText(passwords == 1 ? outerPassword : innerPassword);
                password->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                if (box->objectName() == "externalUpdatePrompt") {
                    editPrompt = true;
                    box->button(QMessageBox::Yes)->click();
                } else if (box->objectName() == "modifiedArchiveQuestion") {
                    if (cleaningUp) {
                        box->button(QMessageBox::No)->click();
                        return;
                    }
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
                    openingMessage += box->text() + '\n';
                    box->accept();
                }
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text() == ToQString(LangString(IDS_CLOSE))) {
                    if (auto *messages =
                            progress->findChild<QTreeWidget *>("IDL_PROGRESS_MESSAGES"))
                        for (int row = 0; row < messages->topLevelItemCount(); ++row)
                            openingMessage += messages->topLevelItem(row)->text(1) + '\n';
                    QTimer::singleShot(0, progress, [this, progress, close] {
                        if (widgetDialogs_.activeDialog() == progress)
                            close->click();
                    });
                } else if (close && outcome >= 3 && outcome <= 4 && !writerCancelled &&
                           !runtime_.writerReceipts().isEmpty()) {
                    writerCancelled = true;
                    close->click();
                }
            }
        });
        responder.start(1);
        const auto closeWindow = qScopeGuard([&] {
            cleaningUp = true;
            responder.start(1);
            window.close();
            for (const auto &path : {editedOutput, nestedSpool})
                if (!path.isEmpty() && QFile::exists(path)) {
                    QFile::remove(path);
                    QDir().rmdir(QFileInfo(path).absolutePath());
                }
        });
        window.openArchive(outer);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QTRY_COMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                     innerName);
        table(window)->selectRow(0);
        table(window)->setCurrentCell(0, 0);
        QTRY_VERIFY(action(window, "openInsideAction")->isEnabled());
        action(window, "openInsideAction")->trigger();
        QTRY_VERIFY2(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C) ==
                         name,
                     qPrintable(openingMessage));
        table(window)->selectRow(0);
        const QString address = window.findChild<QLineEdit *>("archivePath")->text();
        QVERIFY(address.normalized(QString::NormalizationForm_C).contains(innerName));
        QVERIFY(action(window, "editFileAction")->isEnabled());
        action(window, "editFileAction")->trigger();
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        editedOutput = temporary;
        QCOMPARE(read(temporary), QByteArray("original"));
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(QFileInfo::exists(control + ".finished"));
        QTRY_VERIFY2(editPrompt, qPrintable(openingMessage));
        const bool innerFailed = outcome == 1 || outcome == 3;
        if (innerFailed) {
            QTRY_VERIFY(error);
            QCOMPARE(read(temporary), modified);
        } else
            QTRY_VERIFY2(!QFileInfo(temporary).exists(), qPrintable(openingMessage));
        QCOMPARE(read(outer), originalOuter);
        nestedSpool = ToQString(fs2us(static_cast<FileTable *>(table(window))
                                          ->sourcePanel->GetParentFolders()
                                          .Back()
                                          .FilePath));
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
        if (outcome >= 1 && outcome <= 4) {
            QVERIFY(!runtime_.writerReceipts().isEmpty());
            QCOMPARE(writerCancelled, outcome >= 3);
        }
        if (outcome == 0 || outcome == 7) {
            const QString updatedInner = fixture.filePath("updated.7z");
            QVERIFY(write(updatedInner, contents(outer, innerName, outerPassword)));
            QCOMPARE(contents(updatedInner, name, innerPassword), modified);
            QCOMPARE(error, false);
        } else
            QCOMPARE(read(outer), originalOuter);
        if (outerFailed) {
            QVERIFY(error);
            QCOMPARE(contents(nestedSpool, name, innerPassword), modified);
        }
        if (innerFailed)
            QVERIFY(QDir(QFileInfo(temporary).absolutePath()).removeRecursively());
        responder.stop();
    }
    void readOnlyLevelPreventsChainWrites_data() {
        QTest::addColumn<bool>("parentReadOnly");
        QTest::newRow("reader-only-parent") << true;
        QTest::newRow("reader-only-child") << false;
    }
    void readOnlyLevelPreventsChainWrites() {
        QFETCH(bool, parentReadOnly);
        QTemporaryDir fixture;
        const QString source = fixture.filePath("source");
        QVERIFY(QDir().mkpath(source));
        const QString name = QStringLiteral("read only файл.txt");
        const QString innerFormat = "7z";
        const QString outerFormat = parentReadOnly ? "7z" : "zip";
        const QString innerName = parentReadOnly ? "inner.7z" : "inner.fixture7z";
        const QString inner = fixture.filePath(innerName);
        const QString outer = fixture.filePath(parentReadOnly ? "outer.fixture7z" : "outer.zip");
        QVERIFY(write(QDir(source).filePath(name), "original"));
        QVERIFY(createArchive(source, inner, name, innerFormat));
        QVERIFY(createArchive(fixture.path(), outer, innerName, outerFormat));
        const QByteArray originalOuter = read(outer);
        const QString control = fixture.filePath("editor");
        const auto stop = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ArchiveBackend backend(nullptr, readOnlyRuntime_.engine());
        MainWindow window(&backend);
        window.show();
        bool error = false;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog())) {
                error = box->objectName() == "externalUpdateError";
                box->accept();
            }
        });
        responder.start(1);
        window.openArchive(outer);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QTRY_COMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                     innerName);
        table(window)->selectRow(0);
        table(window)->setCurrentCell(0, 0);
        QTRY_VERIFY(action(window, "openInsideAction")->isEnabled());
        action(window, "openInsideAction")->trigger();
        QTRY_COMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                     name);
        table(window)->selectRow(0);
        for (const char *id : {"toolbarAddAction", "deleteAction", "renameAction",
                               "createFolderAction", "moveAction"})
            QVERIFY(!action(window, id)->isEnabled());
        QVERIFY(action(window, "editFileAction")->isEnabled());
        action(window, "editFileAction")->trigger();
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(error);
        QCOMPARE(read(temporary), modified);
        action(window, "upAction")->trigger();
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
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        bool prompted = false;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            auto *modal = widgetDialogs_.activeDialog();
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
                if (close && close->text() == ToQString(LangString(IDS_CLOSE)))
                    QTimer::singleShot(0, progress, [this, progress, close] {
                        if (widgetDialogs_.activeDialog() == progress)
                            close->click();
                    });
            }
        });
        responder.start(1);
        window.openArchive(outer);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QTRY_COMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                     innerName);
        table(window)->selectRow(0);
        table(window)->setCurrentCell(0, 0);
        QTRY_VERIFY(action(window, "openInsideAction")->isEnabled());
        action(window, "openInsideAction")->trigger();
        QVERIFY(prompted);
        QCOMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                 innerName);
        QVERIFY(window.findChild<QLineEdit *>("archivePath")->text().endsWith("outer.zip"));
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
        widgetDialogs_.connectResponder(responder, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog())) {
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
        QTRY_COMPARE(table(window)->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                     innerName);
        table(window)->selectRow(0);
        table(window)->setCurrentCell(0, 0);
        QTRY_VERIFY(action(window, "openInsideAction")->isEnabled());
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
