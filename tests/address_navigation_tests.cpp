#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "layout_inspection.h"
#include "window_state_support.h"

class AddressNavigationTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static bool writeFile(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void missingAddressUsesSourceAncestor_data() {
        QTest::addColumn<unsigned>("sourceIndex");
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("relative");
        QTest::addColumn<bool>("trailing");
        QTest::addColumn<bool>("quoted");
        for (unsigned index : {0u, 1u})
            for (const QString language : {"-", "ru"})
                for (bool relative : {false, true})
                    for (bool trailing : {false, true}) {
                        const QString tag = QString("%1-%2-%3-%4")
                                                .arg(index)
                                                .arg(language)
                                                .arg(relative)
                                                .arg(trailing);
                        QTest::newRow(qPrintable(tag))
                            << index << language << relative << trailing << false;
#ifdef Q_OS_WIN
                        QTest::newRow(qPrintable(tag + "-quoted"))
                            << index << language << relative << trailing << true;
#endif
                    }
    }
    void missingAddressUsesSourceAncestor() {
        QFETCH(unsigned, sourceIndex);
        QFETCH(QString, language);
        QFETCH(bool, relative);
        QFETCH(bool, trailing);
        QFETCH(bool, quoted);
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString parent = fixture.filePath(QStringLiteral("ancestor space δ"));
        const QString child = QDir(parent).filePath(QStringLiteral("inside 文件"));
        const QString peer = fixture.filePath("peer");
        QVERIFY(QDir().mkpath(child) && QDir().mkpath(peer));
        const QString control = QDir(parent).filePath(QStringLiteral("control δ.txt"));
        const QString peerControl = QDir(peer).filePath("peer.txt");
        QVERIFY(writeFile(control, "ancestor control"));
        QVERIFY(writeFile(QDir(child).filePath("old.txt"), "old control"));
        QVERIFY(writeFile(peerControl, "peer control"));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *peerUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - sourceIndex));
        auto *peerAddress = peerUi->findChild<QLineEdit *>("archivePath");
        peerAddress->setFocus();
        peerAddress->setText(peer);
        QTest::keyClick(peerAddress, Qt::Key_Return);
        auto *peerView =
            static_cast<FileTable *>(peerUi->findChild<QTableWidget *>("archiveTable"));
        QTRY_COMPARE(peerView->rowCount(), 1);
        peerView->selectRow(0);
        CMyComPtr<IFolderFolder> peerFolder = peerView->sourcePanel->GetFolder();
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(sourceIndex));
        auto *address = ui->findChild<QLineEdit *>("archivePath");
        auto *view = static_cast<FileTable *>(ui->findChild<QTableWidget *>("archiveTable"));
        address->setFocus();
        address->setText(child);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(view->rowCount(), 1);
        QString input = relative ? QStringLiteral("../missing 文件/deeper")
                                 : QDir(parent).filePath(QStringLiteral("missing 文件/deeper"));
        if (trailing)
            input += '/';
        if (quoted)
            input = '"' + input + '"';
        address->setFocus();
        address->setText(input);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(view->rowCount(), 2);
        QCOMPARE(QDir::cleanPath(QDir::fromNativeSeparators(address->text())), parent);
        QCOMPARE(QDir::cleanPath(QDir::fromNativeSeparators(
                     ToQString(view->sourcePanel->GetCurrentFolderPrefix()))),
                 parent);
        QCOMPARE(QDir::cleanPath(QDir::fromNativeSeparators(window.windowTitle())), parent);
        QTRY_VERIFY(view->hasFocus());
        QVERIFY(view->sourcePanel->ShouldProcessNotifications());
        QCOMPARE(peerView->rowCount(), 1);
        QCOMPARE(peerView->sourcePanel->GetFolder(), (IFolderFolder *)peerFolder);
        QCOMPARE(peerView->selectionModel()->selectedRows().size(), 1);
        for (const auto &pair : {qMakePair(control, QByteArray("ancestor control")),
                                 qMakePair(peerControl, QByteArray("peer control"))}) {
            QFile file(pair.first);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), pair.second);
        }
        QVERIFY(!QFileInfo::exists(QDir(parent).filePath(QStringLiteral("missing 文件"))));
        QCOMPARE(backend.listCalls, 0);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::writeReport(
                window,
                QString("address-missing-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
    }
};

QTEST_MAIN(AddressNavigationTests)
#include "address_navigation_tests.moc"
