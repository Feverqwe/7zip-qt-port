#include "CPP/7zip/UI/FileManager/BrowseDialog2.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include "external_editor_fixture.h"
#include "fixtures/widget_dialogs.h"
#include "qtarchivefoldermanager.h"
#include "qtassociatedfileservices.h"
#include "qtownedtempregistry.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>
#include <atomic>

using namespace ExternalEditorFixture;

namespace {
class CErrorHandle final : public IQtProcessHandle {
    std::atomic_bool &_closed;

  public:
    explicit CErrorHandle(std::atomic_bool &closed) : _closed(closed) {}
    ~CErrorHandle() override { _closed.store(true); }
    UInt32 Id() const override { return 100; }
    UString FileName() const override { return UString(L"owned-fixture"); }
    QtProcessStatus Status() override { return QtProcessStatus::Error; }
};
class CProcessService final : public IQtExternalFileServices {
    QtAssociatedFileServices _native{std::function<bool(const QUrl &)>()};

  public:
    bool FailWait = false;
    HRESULT LaunchResult = S_OK;
    std::atomic_bool Closed{false};
    QString Opened;
    HRESULT StartApplication(const UString &, const UString &, QWidget *) override {
        return E_NOTIMPL;
    }
    IQtProcessServices *ProcessServices() override { return _native.ProcessServices(); }
    HRESULT CreateKnownProcess(const UString &program, const UString &parameters,
                               const UStringVector &paths, QWidget *window,
                               IQtProcessHandle *&process) override {
        Opened = ToQString(paths[0]);
        if (FailWait) {
            process = new CErrorHandle(Closed);
            return LaunchResult;
        }
        return _native.CreateKnownProcess(program, parameters, paths, window, process);
    }
};
QString settings() { return qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"); }
QtOwnedTempEntry ownedDirectory(const QString &file) {
    const QString directory = QFileInfo(file).absolutePath();
    for (const auto &entry : QtOwnedTempRegistry().Entries())
        if (entry.path == directory)
            return entry;
    return {};
}
bool bind(CPanel &panel, IFolderManager *manager, IQtExternalFileServices &service,
          const QString &archive) {
    panel.AttachFolderManager(manager, settings());
    panel.AttachExternalFileServices(&service);
    COpenResult opened;
    if (panel.BindToPath(ToUString(archive), UString(), opened) != S_OK)
        return false;
    if (panel.GetFolder()->LoadItems() != S_OK)
        return false;
    UInt32 count = 0;
    return panel.GetFolder()->GetNumberOfItems(&count) == S_OK && count == 1;
}
} // namespace

class ArchiveExternalLifetimeTests final : public QObject {
    Q_OBJECT
    WidgetDialogFixture widgetDialogs_;
  private slots:
    void init() { QFile::remove(settings()); }
    void childOutlivesParentAndKeepsCopyUntilItsExit() {
        QTemporaryDir fixture;
        const QString name = QStringLiteral("дочерний редактор.txt");
        const QString archive = fixture.filePath("original.zip");
        QVERIFY(write(fixture.filePath(name), "original"));
        QVERIFY(createArchive(fixture.path(), archive, name));
        const QString control = fixture.filePath("child control");
        const auto stop = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control, "--archive-editor-parent");
        ArchiveBackend backend;
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        QtAssociatedFileServices service({});
        CPanel panel;
        QVERIFY(bind(panel, manager, service, archive));
        panel.EditItem(0, true);
        QTRY_VERIFY(QFileInfo(control + ".parent-finished").exists());
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        QVERIFY(QFileInfo(temporary).isFile());
        const auto owned = ownedDirectory(temporary);
        QVERIFY(!owned.key.isEmpty());
        QVERIFY(owned.active && owned.externallyProtected);
        QVERIFY(!QtOwnedTempRegistry().ClaimForDeletion(owned));
        bool cleanupProtected = false;
        QTimer browseResponder;
        connect(&browseResponder, &QTimer::timeout, this, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(widgetDialogs_.activeDialog());
            if (!dialog)
                return;
            auto *list = dialog->findChild<QTreeWidget *>("tempList");
            const auto rows = list->findItems(QFileInfo(owned.path).fileName(), Qt::MatchExactly);
            if (!rows.isEmpty()) {
                list->clearSelection();
                list->setCurrentItem(rows.first());
                rows.first()->setSelected(true);
                cleanupProtected = !dialog->findChild<QPushButton *>("tempDelete")->isEnabled() &&
                                   !rows.first()->toolTip(0).isEmpty();
            }
            dialog->reject();
            browseResponder.stop();
        });
        browseResponder.start(1);
        MyBrowseForTempFolder(nullptr);
        browseResponder.stop();
        QVERIFY(cleanupProtected);
        QVERIFY(QFileInfo(temporary).isFile());
        bool prompt = false;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog())) {
                if (box->objectName() == "externalUpdatePrompt") {
                    prompt = true;
                    box->done(QMessageBox::No);
                }
            }
        });
        responder.start(1);
        // No completion prompt while the child still owns this copy.
        QTest::qWait(100);
        QVERIFY(!prompt);
        QVERIFY(QFileInfo(temporary).isFile());
        QVERIFY(write(control, "change"));
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(QFileInfo(control + ".finished").exists());
        QTRY_VERIFY(prompt);
        QTRY_VERIFY(!QFileInfo(temporary).exists());
        QCOMPARE(contents(archive, name), QByteArray("original"));
        panel.ExitExternalWorkers(false);
    }
    void unchangedLifetime_data() {
        QTest::addColumn<bool>("longRunning");
        QTest::newRow("short-kept-until-owner-exit") << false;
        QTest::newRow("long-deleted-on-process-exit") << true;
    }
    void unchangedLifetime() {
        QFETCH(bool, longRunning);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("неизменённый файл.txt");
        const QString archive = fixture.filePath("original.zip");
        QVERIFY(write(fixture.filePath(name), "original"));
        QVERIFY(createArchive(fixture.path(), archive, name));
        const QString control = fixture.filePath("control");
        const auto stop = qScopeGuard([&] { write(control + ".exit", "exit"); });
        configure(control);
        ArchiveBackend backend;
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        QtAssociatedFileServices service({});
        CPanel panel;
        QVERIFY(bind(panel, manager, service, archive));
        panel.EditItem(0, false);
        QTRY_VERIFY(QFileInfo(control + ".ready").size() > 0);
        const QString temporary = QString::fromUtf8(read(control + ".ready"));
        if (longRunning)
            QTest::qWait(2100);
        QVERIFY(write(control + ".exit", "exit"));
        QTRY_VERIFY(QFileInfo(control + ".finished").exists());
        if (longRunning)
            QTRY_VERIFY(!QFileInfo(temporary).exists());
        else {
            QTest::qWait(100);
            QVERIFY(QFileInfo(temporary).isFile());
        }
        panel.ExitExternalWorkers(false);
        QVERIFY(!QFileInfo(temporary).exists());
        QCOMPARE(contents(archive, name), QByteArray("original"));
    }
    void launchAndWaitFailures_data() {
        QTest::addColumn<bool>("waitError");
        QTest::addColumn<HRESULT>("launchResult");
        QTest::newRow("missing-program-cleanup") << false << HRESULT(S_OK);
        QTest::newRow("wait-error-preserves-copy") << true << HRESULT(S_OK);
        QTest::newRow("launch-error-with-handle-preserves-copy") << true << HRESULT(E_FAIL);
    }
    void launchAndWaitFailures() {
        QFETCH(bool, waitError);
        QFETCH(HRESULT, launchResult);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("ошибка запуска.txt");
        const QString archive = fixture.filePath("original.zip");
        QVERIFY(write(fixture.filePath(name), "original"));
        QVERIFY(createArchive(fixture.path(), archive, name));
        SaveRegEditor(true, ToUString(fixture.filePath("missing program")), settings());
        ArchiveBackend backend;
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CProcessService service;
        service.FailWait = waitError;
        service.LaunchResult = launchResult;
        CPanel panel;
        QVERIFY(bind(panel, manager, service, archive));
        bool error = false;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(widgetDialogs_.activeDialog())) {
                error = box->objectName() == "externalEditorError" &&
                        box->textFormat() == Qt::PlainText;
                box->accept();
            }
        });
        responder.start(1);
        panel.EditItem(0, true);
        responder.stop();
        QVERIFY(!service.Opened.isEmpty());
        if (waitError)
            QTRY_VERIFY(service.Closed.load());
        panel.ExitExternalWorkers(false);
        QCOMPARE(error, !waitError || launchResult != S_OK);
        QCOMPARE(QFileInfo(service.Opened).exists(), waitError);
        if (waitError) {
            const auto owned = ownedDirectory(service.Opened);
            QVERIFY(!owned.key.isEmpty());
            QVERIFY(!owned.active && owned.externallyProtected);
            QVERIFY(!QtOwnedTempRegistry().ClaimForDeletion(owned));
        }
        QCOMPARE(contents(archive, name), QByteArray("original"));
        if (waitError)
            QVERIFY(QDir(QFileInfo(service.Opened).absolutePath()).removeRecursively());
    }
};
int main(int argc, char **argv) {
    const int fixture = ExternalEditorFixture::run(argc, argv);
    if (fixture >= 0)
        return fixture;
    QApplication app(argc, argv);
    ArchiveExternalLifetimeTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "archive_external_lifetime_tests.moc"
