#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "external_editor_fixture.h"
#include "fixtures/widget_dialogs.h"
#include "window_state_support.h"
#include <QScopeGuard>

using namespace ExternalEditorFixture;

class ExtractBackend final : public ArchiveBackend {
  public:
    HRESULT Failure = S_OK;
    QString Output;
    HRESULT lastResult() const override {
        return Failure == S_OK ? ArchiveBackend::lastResult() : Failure;
    }
    void extractSourceEntry(const QString &archive, const QString &entry, const QString &output,
                            const QString &format, const QString &password,
                            bool isAltStream = false) override {
        Output = output;
        if (Failure == S_OK)
            ArchiveBackend::extractSourceEntry(archive, entry, output, format, password,
                                               isAltStream);
        else
            emit finished(false, Failure == E_ABORT
                                     ? QStringLiteral("Операция отменена.")
                                     : QStringLiteral("Controlled extract failure."));
    }
};

class ArchiveExternalSecurityTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture widgetDialogs_;
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void extractionFailureDoesNotLaunchEditor_data() {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("failure") << false;
        QTest::newRow("cancel") << true;
    }
    void extractionFailureDoesNotLaunchEditor() {
        QFETCH(bool, cancel);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("извлечь файл.txt");
        const QString archive = fixture.filePath("original.zip");
        QVERIFY(write(fixture.filePath(name), "original"));
        QVERIFY(createArchive(fixture.path(), archive, name));
        const QByteArray original = read(archive);
        const QString control = fixture.filePath("control");
        configure(control);
        ExtractBackend backend;
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        backend.Failure = cancel ? E_ABORT : E_FAIL;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [this] {
            auto *modal = widgetDialogs_.activeDialog();
            if (auto *box = qobject_cast<QMessageBox *>(modal))
                box->accept();
            else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text().contains(QStringLiteral("Close")))
                    close->click();
            }
        });
        responder.start(1);
        action(window, "editFileAction")->trigger();
        responder.stop();
        QVERIFY(!backend.Output.isEmpty());
        QVERIFY(!QFileInfo(control + ".ready").exists());
        QVERIFY(!QFileInfo(backend.Output).exists());
        QCOMPARE(read(archive), original);
    }
    void protectedArchiveUsesSourcePasswordAndWriter_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("cancelPassword");
        QTest::newRow("zip-aes-update") << QString("zip") << false;
        QTest::newRow("7z-headers-update") << QString("7z") << false;
        QTest::newRow("zip-extract-password-cancel") << QString("zip") << true;
    }
    void protectedArchiveUsesSourcePasswordAndWriter() {
        QFETCH(QString, format);
        QFETCH(bool, cancelPassword);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("защищённый файл.txt");
        const QString archive = fixture.filePath("original." + format);
        const QString password = format == "zip" ? QStringLiteral("fixture password space")
                                                 : QStringLiteral("fixture 密碼 space");
        QVERIFY(write(fixture.filePath(name), "original"));
        QVERIFY(createArchive(fixture.path(), archive, name, format, password));
        const QByteArray original = read(archive);
        const QString control = fixture.filePath("control");
        const auto stop = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ArchiveBackend backend;
        MainWindow window(&backend);
        bool extracting = false, asked = false;
        int passwordPrompts = 0;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            auto *modal = widgetDialogs_.activeDialog();
            if (auto *dialog = dynamic_cast<CPasswordDialog *>(modal)) {
                ++passwordPrompts;
                if (cancelPassword && extracting)
                    dialog->reject();
                else {
                    dialog->findChild<QLineEdit *>("passwordEdit")->setText(password);
                    dialog->accept();
                }
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                if (box->objectName() == "externalUpdatePrompt") {
                    asked = true;
                    box->button(QMessageBox::Yes)->click();
                } else
                    box->accept();
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text().contains(QStringLiteral("Close")))
                    close->click();
            }
        });
        responder.start(1);
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        extracting = true;
        action(window, "editFileAction")->trigger();
        if (cancelPassword) {
            QVERIFY(passwordPrompts > 0);
            QVERIFY(!QFileInfo(control + ".ready").exists());
            QCOMPARE(read(archive), original);
        } else {
            QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
            const QString temporary = QString::fromUtf8(read(control + ".ready"));
            QEventLoop completion;
            QTimer completed;
            connect(&completed, &QTimer::timeout, &completion, [&] {
                if (asked && !QFileInfo(temporary).exists())
                    completion.quit();
            });
            completed.start(5);
            QTimer::singleShot(5000, &completion, &QEventLoop::quit);
            QVERIFY(write(control, "change"));
            QVERIFY(write(control + ".exit", "exit"));
            // Cocoa must deliver nested modal requests from a running loop,
            // rather than QTest's manual processEvents wait.
            completion.exec();
            QVERIFY(asked);
            QVERIFY(!QFileInfo(temporary).exists());
            QVERIFY(passwordPrompts > 0);
            QCOMPARE(contents(archive, name, password), modified);
            QProcess wrong;
            wrong.start(executable(), {"t", "-pwrong-fixture-password", "--", archive});
            QVERIFY(wrong.waitForFinished(10000));
            QVERIFY(wrong.exitCode() != 0);
        }
    }
};
int main(int argc, char **argv) {
    const int fixture = ExternalEditorFixture::run(argc, argv);
    if (fixture >= 0)
        return fixture;
    QApplication app(argc, argv);
    ArchiveExternalSecurityTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "archive_external_security_tests.moc"
