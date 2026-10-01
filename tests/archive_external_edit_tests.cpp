#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "external_editor_fixture.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QCoreApplication>
#include <QScopeGuard>
#include <QStandardPaths>

using namespace ExternalEditorFixture;

class UpdateBackend final : public ArchiveBackend {
  public:
    bool Fail = false;
    bool Started = false;
    bool HoldBusy = false;
    bool busy() const override { return HoldBusy || ArchiveBackend::busy(); }
    void updateSourceFile(const QString &archive, const QString &entry, const QString &root,
                          const QString &output, const QString &format,
                          const QString &password) override {
        Started = true;
        if (Fail)
            emit finished(false, QStringLiteral("Controlled external update failure."));
        else
            ArchiveBackend::updateSourceFile(archive, entry, root, output, format, password);
    }
};

class ArchiveExternalEditTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void changedFileUsesNativeUpdatePrompt_data() {
        QTest::addColumn<int>("answer");
        QTest::addColumn<bool>("viewer");
        QTest::newRow("editor-yes") << int(QMessageBox::Yes) << false;
        QTest::newRow("editor-no") << int(QMessageBox::No) << false;
        QTest::newRow("editor-cancel") << int(QMessageBox::Cancel) << false;
        QTest::newRow("viewer-yes") << int(QMessageBox::Yes) << true;
    }
    void changedFileUsesNativeUpdatePrompt() {
        QFETCH(int, answer);
        QFETCH(bool, viewer);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("файл с пробелом.txt");
        QVERIFY(write(fixture.filePath(name), "original"));
        const QString archive = fixture.filePath("original.zip");
        QVERIFY(createArchive(fixture.path(), archive, name));
        const QString control = fixture.filePath("editor control");
        const auto stopEditor = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.resize(viewer ? QSize(640, 400) : QSize(980, 620));
        window.show();
        window.activateWindow();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        window.activateWindow();
        QApplication::processEvents();
        table(window)->setFocus();
        QTRY_VERIFY(table(window)->hasFocus());
        QTRY_VERIFY(action(window, viewer ? "viewFileAction" : "editFileAction")->isEnabled());
        QTest::keyClick(table(window), viewer ? Qt::Key_F3 : Qt::Key_F4);
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        QVERIFY(QFileInfo(temporary).isFile());
        QCOMPARE(QFileInfo(temporary).fileName().normalized(QString::NormalizationForm_C), name);
        const QString temporaryDirectory = QFileInfo(temporary).absolutePath();
        bool asked = false;
        bool geometry = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, &window, [&] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (box && box->objectName() == "externalUpdatePrompt") {
                responder.stop();
                asked = box->textFormat() == Qt::PlainText;
                geometry = true;
                for (auto *button : box->findChildren<QAbstractButton *>())
                    if (button->isVisible())
                        geometry &= UiInspection::fullyVisible(*button, *box);
                for (auto *label : box->findChildren<QLabel *>())
                    if (label->isVisible())
                        geometry &= UiInspection::fullyVisible(*label, *box) &&
                                    UiInspection::labelFits(*label);
                geometry &= UiInspection::writeReport(*box, QString("external-update-prompt-%1-%2")
                                                                .arg(viewer ? "compact" : "normal")
                                                                .arg(answer));
                if (answer == QMessageBox::Cancel)
                    QTest::keyClick(box, Qt::Key_Escape);
                else
                    box->button(QMessageBox::StandardButton(answer))->click();
            }
        });
        responder.start(1);
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(asked);
        QVERIFY(geometry);
        QTRY_VERIFY(!QDir(temporaryDirectory).exists());
        responder.stop();
        QCOMPARE(contents(archive, name), answer == QMessageBox::Yes ? modified : "original");
        QTRY_COMPARE(table(window)->rowCount(), 1);
        if (answer == QMessageBox::Yes) {
            QCOMPARE(table(window)->item(0, 2)->text(), QString::number(modified.size()));
            QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        }
    }
    void closingWhileKnownProcessRunsRetainsTemporaryCopy() {
        QTemporaryDir fixture;
        const QString name = QStringLiteral("путь с пробелом.txt");
        QVERIFY(write(fixture.filePath(name), "original"));
        const QString archive = fixture.filePath("original.zip");
        QVERIFY(createArchive(fixture.path(), archive, name));
        const QString control = fixture.filePath("control");
        const auto stopEditor = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ArchiveBackend backend;
        auto window = std::make_unique<MainWindow>(&backend);
        window->show();
        window->openArchive(archive);
        QTRY_COMPARE(table(*window)->rowCount(), 1);
        table(*window)->selectRow(0);
        action(*window, "editFileAction")->trigger();
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        const QString temporaryDirectory = QFileInfo(temporary).absolutePath();
        window.reset();
        QVERIFY(QFileInfo(temporary).isFile());
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(QFileInfo(control + ".finished").exists());
        QCOMPARE(read(temporary), modified);
        QCOMPARE(contents(archive, name), QByteArray("original"));
        QVERIFY(QDir(temporaryDirectory).removeRecursively());
    }
    void refusedUpdateRetainsEditedCopy_data() {
        QTest::addColumn<int>("outcome");
        QTest::newRow("update-failure") << 1;
        QTest::newRow("update-cancel") << 2;
        QTest::newRow("read-only-archive") << 3;
    }
    void refusedUpdateRetainsEditedCopy() {
        QFETCH(int, outcome);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("сохранить правку.txt");
        const QString format = outcome == 3 ? "tar" : "zip";
        const QString archive = fixture.filePath("original." + format);
        QVERIFY(write(fixture.filePath(name), "original"));
        QVERIFY(createArchive(fixture.path(), archive, name, format));
        const QByteArray originalArchive = read(archive);
        const QString control = fixture.filePath("control");
        const auto stopEditor = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        UpdateBackend backend;
        backend.Fail = outcome == 1;
        connect(&backend, &ArchiveBackend::engineStarted, &backend, [&] {
            if (outcome == 2 && backend.Started)
                backend.cancel();
        });
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QVERIFY(action(window, "editFileAction")->isEnabled());
        action(window, "editFileAction")->trigger();
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        bool error = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, &window, [&] {
            auto *modal = QApplication::activeModalWidget();
            if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                if (box->objectName() == "externalUpdatePrompt")
                    box->done(QMessageBox::Yes);
                else {
                    error = box->objectName() == "externalUpdateError";
                    box->accept();
                }
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text().contains(QStringLiteral("Close")))
                    close->click();
            }
        });
        responder.start(1);
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(error);
        responder.stop();
        QCOMPARE(backend.Started, outcome != 3);
        QVERIFY(!backend.busy());
        QCOMPARE(read(temporary), modified);
        QCOMPARE(read(archive), originalArchive);
        QCOMPARE(contents(archive, name), QByteArray("original"));
        QVERIFY(QDir(QFileInfo(temporary).absolutePath()).removeRecursively());
    }
    void deferredUpdateAndOwnerChanges_data() {
        QTest::addColumn<int>("outcome");
        QTest::newRow("wait-for-idle") << 0;
        QTest::newRow("close-pending") << 1;
        QTest::newRow("changed-location") << 2;
        QTest::newRow("other-focused-panel") << 3;
    }
    void deferredUpdateAndOwnerChanges() {
        QFETCH(int, outcome);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("отложенная правка.txt");
        const QString archive = fixture.filePath("original.zip");
        QVERIFY(write(fixture.filePath(name), "original"));
        QVERIFY(createArchive(fixture.path(), archive, name));
        const QByteArray originalArchive = read(archive);
        const QString control = fixture.filePath("control");
        const auto stopEditor = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        UpdateBackend backend;
        auto window = std::make_unique<MainWindow>(&backend);
        window->show();
        window->openArchive(archive);
        QTRY_COMPARE(table(*window)->rowCount(), 1);
        auto *leftPanel = window->findChild<QWidget *>("filePanel0");
        table(*window)->selectRow(0);
        action(*window, "editFileAction")->trigger();
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        QLineEdit *rightAddress = nullptr;
        if (outcome == 3) {
            action(*window, "twoPanelsAction")->trigger();
            QVERIFY(action(*window, "twoPanelsAction")->isChecked());
            rightAddress =
                window->findChild<QWidget *>("filePanel1")->findChild<QLineEdit *>("archivePath");
            rightAddress->setFocus();
            rightAddress->setText(fixture.path());
            QTest::keyClick(rightAddress, Qt::Key_Return);
        }
        backend.HoldBusy = true;
        bool asked = false, failed = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                if (box->objectName() == "externalUpdatePrompt") {
                    asked = true;
                    box->done(QMessageBox::Yes);
                } else if (box->objectName() == "externalUpdateError") {
                    failed = true;
                    box->accept();
                }
            }
        });
        responder.start(1);
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(QFileInfo(control + ".finished").exists());
        QTest::qWait(100);
        QVERIFY(!asked && !backend.Started);
        QVERIFY(QFileInfo(temporary).isFile());
        if (outcome == 1) {
            window.reset();
            backend.HoldBusy = false;
            QApplication::processEvents();
            QVERIFY(!asked && !backend.Started);
        } else {
            if (outcome == 2) {
                auto *address = leftPanel->findChild<QLineEdit *>("archivePath");
                address->setText(fixture.path());
                QTest::keyClick(address, Qt::Key_Return);
            }
            backend.HoldBusy = false;
            QTRY_VERIFY(asked);
            if (outcome == 2)
                QTRY_VERIFY(failed);
            else {
                QTRY_VERIFY(!QFileInfo(temporary).exists());
                QCOMPARE(contents(archive, name), modified);
                if (outcome == 3)
                    QCOMPARE(QDir::cleanPath(rightAddress->text()), fixture.path());
                QCOMPARE(leftPanel->findChild<QTableWidget *>("archiveTable")->item(0, 2)->text(),
                         QString::number(modified.size()));
            }
        }
        responder.stop();
        if (outcome == 1 || outcome == 2) {
            QCOMPARE(read(archive), originalArchive);
            QCOMPARE(read(temporary), modified);
            QVERIFY(QDir(QFileInfo(temporary).absolutePath()).removeRecursively());
        }
    }
};

int main(int argc, char **argv) {
    const int fixture = ExternalEditorFixture::run(argc, argv);
    if (fixture >= 0)
        return fixture;
    QApplication app(argc, argv);
    ArchiveExternalEditTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "archive_external_edit_tests.moc"
