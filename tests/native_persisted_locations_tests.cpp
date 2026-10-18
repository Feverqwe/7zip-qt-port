#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "fixtures/native_network_share.h"
#include "fixtures/widget_dialogs.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QScopeGuard>
#include <QTreeWidget>

class NativePersistedLocationsTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture dialogs_;

    static QWidget *pane(MainWindow &window, unsigned index) {
        return window.findChild<QWidget *>(QString("filePanel%1").arg(index));
    }
    static FileTable *list(MainWindow &window, unsigned index) {
        return static_cast<FileTable *>(
            pane(window, index)->findChild<QTableWidget *>("archiveTable"));
    }
    static QString prefix(MainWindow &window, unsigned index) {
        return ToQString(list(window, index)->sourcePanel->GetCurrentFolderPrefix());
    }
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
    static void reportNamespace(MainWindow &window, unsigned owner) {
        auto *address = pane(window, owner)->findChild<QLineEdit *>("archivePath");
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*address, window));
            QVERIFY(UiInspection::fullyVisible(*list(window, owner), window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("native-persisted-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
    }
    void dismissUnexpectedErrors(QTimer &timer, MainWindow &window, QStringList &errors) {
        dialogs_.connectResponder(timer, &window, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(dialogs_.activeDialog())) {
                errors << box->text();
                box->accept();
            }
        });
        timer.start(5);
    }
    void openHistory(MainWindow &window, const QString &source) {
        bool selected = false;
        QTimer responder;
        QTimer deadline;
        deadline.setSingleShot(true);
        connect(&deadline, &QTimer::timeout, &window, [&] {
            if (auto *dialog = window.findChild<QDialog *>("foldersHistoryDialog"))
                dialog->reject();
        });
        dialogs_.connectResponder(responder, &window, [&] {
            auto *dialog = dialogs_.activeDialog();
            if (!dialog || dialog->objectName() != "foldersHistoryDialog")
                return;
            responder.stop();
            const auto close = qScopeGuard([&] {
                if (dialog->isVisible())
                    dialog->reject();
            });
            auto *tree = dialog->findChild<QTreeWidget *>("foldersHistoryList");
            QVERIFY(tree);
            const auto matches = tree->findItems(source, Qt::MatchExactly);
            QCOMPARE(matches.size(), 1);
            tree->setCurrentItem(matches.first());
            selected = true;
            dialog->accept();
        });
        responder.start(5);
        deadline.start(5000);
        action(window, "foldersHistoryAction")->trigger();
        QVERIFY(selected);
    }

  private slots:
    void init() {
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QVERIFY(!settings.isEmpty());
        QFile::remove(settings);
    }
    void namespaceBookmarksAndHistory_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<unsigned>("owner");
        QTest::addColumn<QString>("language");
        for (const char *kind : {"device", "ads", "unc"})
            for (unsigned owner : {0u, 1u})
                for (const char *language : {"-", "ru"})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(kind).arg(owner).arg(language)))
                        << QString::fromLatin1(kind) << owner << QString::fromLatin1(language);
    }
    void namespaceBookmarksAndHistory() {
        QFETCH(QString, kind);
        QFETCH(unsigned, owner);
        QFETCH(QString, language);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString base = fixture.filePath(QStringLiteral("base файл 空.txt"));
        const QString stream = base + QStringLiteral(":stream 空");
        QVERIFY(write(base, QByteArray("base\0bytes", 10)));
        if (kind == "ads")
            QVERIFY(write(stream, QByteArray("stream\0bytes", 12)));
        NativeNetworkShare share;
        if (kind == "unc") {
            const QString error = share.create();
            QVERIFY2(error.isEmpty(), qPrintable(error));
        }
        const QString requested = kind == "device" ? QStringLiteral("\\\\.\\")
                                  : kind == "ads"  ? base + ':'
                                                   : share.server();
        ControlledBackend backend;
        MainWindow window(&backend);
        QStringList errors;
        QTimer errorResponder;
        dismissUnexpectedErrors(errorResponder, window, errors);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *address = pane(window, owner)->findChild<QLineEdit *>("archivePath");
        auto *peer = pane(window, 1 - owner)->findChild<QLineEdit *>("archivePath");
        const QString peerDraft = QStringLiteral("peer черновик 空");
        peer->setText(peerDraft);
        address->setFocus();
        QTRY_VERIFY(address->hasFocus());
        address->setText(requested);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        const QString source = prefix(window, owner);
        QVERIFY(!source.isEmpty());
        QCOMPARE(QDir::toNativeSeparators(source).compare(QDir::toNativeSeparators(requested),
                                                          Qt::CaseInsensitive),
                 0);
        const auto &panel = *list(window, owner)->sourcePanel;
        QVERIFY(kind == "device" ? panel.IsFSDrivesFolder()
                : kind == "ads"  ? panel.IsAltStreamsFolder()
                                 : panel.IsNetFolder());
        QVERIFY(action(window, "saveBookmark5Action")->isEnabled());
        action(window, "saveBookmark5Action")->trigger();
        CFastFolders folders;
        folders.Read(settings);
        QCOMPARE(ToQString(folders.GetString(5)), source);
        action(window, "openRootAction")->trigger();
        QVERIFY(prefix(window, owner) != source);
        action(window, "openBookmark5Action")->trigger();
        QTRY_COMPARE(prefix(window, owner), source);
        action(window, "openRootAction")->trigger();
        openHistory(window, source);
        QTRY_COMPARE(prefix(window, owner), source);
        QCOMPARE(peer->text(), peerDraft);
        reportNamespace(window, owner);
        ControlledBackend restartedBackend;
        MainWindow restarted(&restartedBackend);
        action(restarted, "openBookmark5Action")->trigger();
        QTRY_COMPARE(prefix(restarted, 0), source);
        QCOMPARE(read(base), QByteArray("base\0bytes", 10));
        if (kind == "ads")
            QCOMPARE(read(stream), QByteArray("stream\0bytes", 12));
        if (kind == "unc") {
            QCOMPARE(read(share.filePath()), NativeNetworkShare::payload());
            QCOMPARE(share.remove(), 0ul);
        }
        QCOMPARE(backend.listCalls, 0);
        QCOMPARE(backend.extractCalls, 0);
        QCOMPARE(restartedBackend.listCalls, 0);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
    }
};

QTEST_MAIN(NativePersistedLocationsTests)

#include "native_persisted_locations_tests.moc"
