#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "external_editor_fixture.h"
#include "fixtures/widget_dialogs.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTreeWidget>

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
    WidgetDialogFixture widgetDialogs_;
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void sourceViewerOpensLargeFileWithoutPreviewLimit() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("большой файл 密碼.bin");
        const QByteArray bytes(17 * 1024 * 1024, 'x');
        QVERIFY(write(fixture.filePath(name), bytes));
        const QString archive = fixture.filePath("large.zip");
        QVERIFY(createArchive(fixture.path(), archive, name));
        const QByteArray originalArchive = read(archive);
        const QString control = fixture.filePath("viewer control");
        const auto stopViewer = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ArchiveBackend backend;
        MainWindow window(&backend);
        QString unexpectedMessage;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, &window, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog())) {
                unexpectedMessage = box->text();
                box->accept();
            }
        });
        responder.start(1);
        window.show();
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        table(window)->setCurrentCell(0, 0);
        QTRY_VERIFY(action(window, "viewFileAction")->isEnabled());
        action(window, "viewFileAction")->trigger();
        QVERIFY2(unexpectedMessage.isEmpty(), qPrintable(unexpectedMessage));
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        QCOMPARE(read(temporary), bytes);
        // Original MyThreadFunction retains a short unchanged launch until
        // panel exit. Exercise its long-running process-completion branch.
        QTest::qWait(2100);
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(!QFileInfo::exists(temporary));
        QCOMPARE(read(archive), originalArchive);
    }
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
        QEventLoop promptDelivery;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, &window, [&] {
            auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog());
            if (box && box->objectName() == "externalUpdatePrompt") {
                responder.stop();
                connect(box, &QObject::destroyed, &promptDelivery, &QEventLoop::quit);
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
        // Cocoa native modal delivery requires a running event loop, rather
        // than QTest's manual processEvents pump. Keep the same 5s wait budget.
        QTimer::singleShot(5000, &promptDelivery, &QEventLoop::quit);
        if (!asked)
            promptDelivery.exec();
        QVERIFY2(
            asked,
            qPrintable(QString("editor finished=%1, temporary bytes=%2, backend busy=%3, modal=%4")
                           .arg(QFileInfo::exists(control + ".finished"))
                           .arg(QFileInfo(temporary).size())
                           .arg(backend.busy())
                           .arg(QApplication::activeModalWidget()
                                    ? QApplication::activeModalWidget()->objectName()
                                    : QStringLiteral("none"))));
        QVERIFY(geometry);
        QTRY_VERIFY(!QDir(temporaryDirectory).exists());
        responder.stop();
        QCOMPARE(contents(archive, name),
                 answer == QMessageBox::Yes ? modified : QByteArray("original"));
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
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("russian");
        QTest::addColumn<bool>("icons");
        QTest::addColumn<bool>("alternative");
        for (int outcome : {1, 2, 3})
            for (unsigned pane : {0u, 1u})
                for (bool russian : {false, true})
                    for (bool icons : {false, true})
                        for (bool alternative : {false, true})
                            QTest::newRow(qPrintable(QString("%1-%2-%3-%4-%5")
                                                         .arg(outcome)
                                                         .arg(pane)
                                                         .arg(russian)
                                                         .arg(icons)
                                                         .arg(alternative)))
                                << outcome << pane << russian << icons << alternative;
    }
    void refusedUpdateRetainsEditedCopy() {
        QFETCH(int, outcome);
        QFETCH(unsigned, pane);
        QFETCH(bool, russian);
        QFETCH(bool, icons);
        QFETCH(bool, alternative);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegLang(UString(russian ? L"ru" : L"-"), settings);
        ReloadLang(settings);
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
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *owner = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *list = static_cast<FileTable *>(owner->findChild<QTableWidget *>("archiveTable"));
        owner->findChild<QLineEdit *>("archivePath")->setFocus();
        window.openArchive(archive);
        QTRY_COMPARE(list->rowCount(), 1);
        action(window, icons ? "viewMode0Action" : "viewMode3Action")->trigger();
        list->selectRow(0);
        list->sourcePanel->SetAlternativeSelection(alternative);
        list->sourcePanel->Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> provider = list->sourcePanel->GetFolder();
        auto *peer = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        auto *peerList = static_cast<FileTable *>(peer->findChild<QTableWidget *>("archiveTable"));
        peerList->sourcePanel->Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> peerProvider = peerList->sourcePanel->GetFolder();
        const int peerRows = peerList->rowCount();
        QVERIFY(action(window, "editFileAction")->isEnabled());
        bool error = false;
        QEventLoop errorDelivery;
        QPointer<QMessageBox> closingMessage;
        QString previousModalState;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, &window, [&] {
            if (closingMessage)
                return;
            auto *modal = widgetDialogs_.activeDialog();
            if (modal) {
                const auto *button = modal->findChild<QPushButton *>("IDCANCEL");
                const QString state =
                    QString("%1/%2/%3")
                        .arg(modal->metaObject()->className(), modal->objectName(),
                             button ? button->text() : QString());
                if (state != previousModalState) {
                    previousModalState = state;
                    qInfo().noquote() << "External update modal:" << state;
                    if (auto *messages = modal->findChild<QTreeWidget *>("IDL_PROGRESS_MESSAGES"))
                        for (int row = 0; row < messages->topLevelItemCount(); ++row)
                            qInfo().noquote()
                                << "Progress message:" << messages->topLevelItem(row)->text(1);
                }
            }
            if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                closingMessage = box;
                qInfo().noquote() << "External update message:" << box->text();
                if (box->objectName() == "externalUpdatePrompt")
                    box->done(QMessageBox::Yes);
                else {
                    error |= box->objectName() == "externalUpdateError";
                    if (error)
                        connect(box, &QObject::destroyed, &errorDelivery, &QEventLoop::quit);
                    box->accept();
                }
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text() == ToQString(LangString(IDS_CLOSE)))
                    QTimer::singleShot(0, progress, [this, progress, close] {
                        // Source sets the Close caption before presenting its
                        // final error. Allow that transition to finish before
                        // simulating input; never click through a child modal.
                        if (widgetDialogs_.activeDialog() == progress)
                            close->click();
                    });
            }
        });
        responder.start(1);
        action(window, "editFileAction")->trigger();
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        auto *peerAddress = peer->findChild<QLineEdit *>("archivePath");
        peerAddress->setFocus();
        QWidget *const focused = window.focusWidget();
        QVERIFY(focused && peer->isAncestorOf(focused));
        QSignalSpy refreshed(list->model(), &QAbstractItemModel::rowsRemoved);
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTimer::singleShot(5000, &errorDelivery, &QEventLoop::quit);
        errorDelivery.exec();
        QVERIFY(error);
        responder.stop();
        QCOMPARE(backend.Started, outcome != 3);
        QVERIFY(!backend.busy());
        QCOMPARE(read(temporary), modified);
        QCOMPARE(read(archive), originalArchive);
        QCOMPARE(contents(archive, name), QByteArray("original"));
        QCOMPARE(refreshed.isEmpty(), outcome == 3);
        QCOMPARE(list->rowCount(), 1);
        QCOMPARE(list->selectionModel()->selectedRows().size(), 1);
        CRecordVector<UInt32> marks;
        list->sourcePanel->Get_ItemIndices_Selected(marks);
        QCOMPARE(marks.Size(), 1u);
        QVERIFY(list->updatesEnabled());
        QVERIFY(list->isEnabled());
        QCOMPARE(list->sourcePanel->GetFolder(), (IFolderFolder *)provider);
        QCOMPARE(peerList->sourcePanel->GetFolder(), (IFolderFolder *)peerProvider);
        QCOMPARE(peerList->rowCount(), peerRows);
        QTRY_COMPARE(window.focusWidget(), focused);
        QWidget *visible =
            icons ? static_cast<QWidget *>(owner->findChild<QListView *>("iconList")) : list;
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*visible, window));
            QVERIFY(UiInspection::fullyVisible(*peerList, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("external-refresh-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
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
        QEventLoop completion;
        QPointer<QMessageBox> closingMessage;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            if (closingMessage)
                return;
            if (auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog())) {
                closingMessage = box;
                if (box->objectName() == "externalUpdatePrompt") {
                    asked = true;
                    box->done(QMessageBox::Yes);
                } else if (box->objectName() == "externalUpdateError") {
                    failed = true;
                    connect(box, &QObject::destroyed, &completion, &QEventLoop::quit);
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
            QTimer published;
            connect(&published, &QTimer::timeout, &completion, [&] {
                if (!QFileInfo(temporary).exists())
                    completion.quit();
            });
            published.start(2);
            QTimer::singleShot(5000, &completion, &QEventLoop::quit);
            backend.HoldBusy = false;
            completion.exec();
            QVERIFY(asked);
            if (outcome == 2)
                QVERIFY(failed);
            else {
                QVERIFY(!QFileInfo(temporary).exists());
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
