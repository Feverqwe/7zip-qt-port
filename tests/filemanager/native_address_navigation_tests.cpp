#include "7zip/UI/FileManager/resource.h"
#include "filemanager/support/window_state_support.h"
#include "filemanager/views/filetable.h"
#include "localization/LangUtils.h"
#include "settings/RegistryUtils.h"
#include "support/native_network_share.h"
#include "support/native_shell_icon.h"
#include "support/widget_dialogs.h"
#include "support/window_activation.h"
#include "ui/support/address_popup_support.h"
#include "ui/support/layout_inspection.h"
#include <QFileInfo>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QToolButton>

class NativeAddressNavigationTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture dialogs_;

    static QWidget *pane(MainWindow &window, unsigned owner) {
        return window.findChild<QWidget *>(QString("filePanel%1").arg(owner));
    }
    static FileTable *view(MainWindow &window, unsigned owner) {
        return static_cast<FileTable *>(
            pane(window, owner)->findChild<FileTable *>("archiveTable"));
    }
    static QString prefix(MainWindow &window, unsigned owner) {
        return ToQString(view(window, owner)->sourcePanel->GetCurrentFolderPrefix());
    }
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
    static void enter(QLineEdit &address, const QString &path) {
        address.setFocus();
        QTRY_VERIFY(address.hasFocus());
        address.setText(path);
        QTest::keyClick(&address, Qt::Key_Return);
    }
    static void inspect(MainWindow &window, unsigned owner, const QString &phase) {
        auto *address = pane(window, owner)->findChild<QLineEdit *>("archivePath");
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*address, window));
            QVERIFY(UiInspection::fullyVisible(*view(window, owner), window));
            const auto buttons = address->findChildren<QToolButton *>();
            QVERIFY(!buttons.isEmpty());
            for (const auto *button : buttons)
                QVERIFY(UiInspection::lineEditIconVisible(*button, window));
            const QImage expected =
                NativeShellIcon::image(prefix(window, owner), FILE_ATTRIBUTE_DIRECTORY);
            QVERIFY(!expected.isNull());
            auto *icon = address->findChild<QAction *>("currentAddressIcon");
            QVERIFY(icon);
            QCOMPARE(icon->icon().pixmap(16, 16).toImage(), expected);
            // Opt-in icon review captures only the affected closed editor.
            QWidget &reportRoot = qEnvironmentVariableIntValue("SEVENZIP_CAPTURE_SCREENSHOTS") == 1
                                      ? static_cast<QWidget &>(*address)
                                      : window;
            QVERIFY(UiInspection::writeReport(reportRoot, QString("native-address-%1-%2-%3")
                                                              .arg(QTest::currentDataTag(), phase)
                                                              .arg(size.width())));
        }
    }
    static void rootAndEscape(MainWindow &window, unsigned owner) {
        auto *address = pane(window, owner)->findChild<QLineEdit *>("archivePath");
        const QString before = prefix(window, owner);
        address->setFocus();
        QTRY_VERIFY(address->hasFocus());
        address->setText(QStringLiteral("uncommitted адрес 空"));
        QTest::keyClick(address, Qt::Key_Escape);
        QCOMPARE(address->text(), before);
        QTRY_VERIFY(view(window, owner)->hasFocus());
        action(window, "openRootAction")->trigger();
        QTRY_COMPARE(prefix(window, owner),
                     QString::fromWCharArray(LangString(IDS_COMPUTER)) + '\\');
        action(window, "openRootAction")->trigger();
        action(window, "parentFolderAction")->trigger();
        QTRY_VERIFY(prefix(window, owner).isEmpty());
        action(window, "backAction")->trigger();
        QTRY_COMPARE(prefix(window, owner), before);
    }
    static void selectAncestor(MainWindow &window, unsigned owner, const QString &kind,
                               const QString &ancestor) {
        auto *combo = pane(window, owner)->findChild<QComboBox *>("pathCombo");
        QVERIFY(combo);
        combo->lineEdit()->setFocus();
        QTRY_VERIFY(combo->lineEdit()->hasFocus());
        const QString before = combo->currentText();
        AddressPopupSupport::show(*combo);
        QTRY_VERIFY(combo->view()->isVisible());
        QCOMPARE(combo->currentText(), before);
        checkGrouping(*combo);
        QVERIFY(!QTest::currentTestFailed());
        int row = 0;
        if (kind == "ads")
            row = combo->findText(
                QFileInfo(QDir::cleanPath(QDir::fromNativeSeparators(ancestor))).fileName());
        else {
            QString label = ancestor;
            if (label.endsWith('\\'))
                label.chop(1);
            QCOMPARE(combo->itemText(0), label);
        }
        QVERIFY(row >= 0);
        QVERIFY(!combo->itemIcon(row).isNull());
        AddressPopupSupport::selectRow(*combo, row);
        QVERIFY(!QTest::currentTestFailed());
        QTRY_COMPARE(QDir::toNativeSeparators(prefix(window, owner)).toCaseFolded(),
                     QDir::toNativeSeparators(ancestor).toCaseFolded());
        QTRY_VERIFY(view(window, owner)->hasFocus());
    }
    static void checkGrouping(QComboBox &combo) {
        const int documents = combo.findText(QString::fromWCharArray(LangString(IDS_DOCUMENTS)));
        const int computer = combo.findText(QString::fromWCharArray(LangString(IDS_COMPUTER)));
        const int network = combo.findText(QString::fromWCharArray(LangString(IDS_NETWORK)));
        QVERIFY(documents > 0);
        QCOMPARE(computer, documents + 1);
        QCOMPARE(network, combo.count() - 1);
        const auto drives = QDir::drives();
        QCOMPARE(network, computer + 1 + int(drives.size()));
        for (int i = 0; i < int(drives.size()); ++i) {
            QString name = QDir::toNativeSeparators(drives[i].absoluteFilePath());
            if (name.endsWith('\\'))
                name.chop(1);
            QCOMPARE(combo.itemText(computer + 1 + i), name);
        }
        QStyleOptionViewItem option;
        option.initFrom(combo.view());
        option.widget = combo.view();
        QStyledItemDelegate plain;
        for (int row = 0; row < combo.count(); ++row) {
            const int indent = row < documents ? row : row > computer && row < network ? 1 : 0;
            const auto index = combo.model()->index(row, 0);
            const int extra = combo.itemDelegate()->sizeHint(option, index).width() -
                              plain.sizeHint(option, index).width();
            QCOMPARE(extra, indent * option.fontMetrics.height());
        }
    }
    static void refuseRemovedShare(MainWindow &window, unsigned owner, NativeNetworkShare &share,
                                   const QString &server, const QStringList &errors) {
        auto *list = view(window, owner);
        int row = -1;
        for (int i = 0; i < list->rowCount(); ++i)
            if (list->model()->index(i, 0).data().toString() == share.name())
                row = i;
        QVERIFY(row >= 0);
        QCOMPARE(share.remove(), 0ul);
        list->setCurrentCell(row, 0);
        list->selectRow(row);
        list->setFocus();
        QTRY_VERIFY(list->hasFocus());
        QTest::keyClick(list, Qt::Key_Return);
        QTRY_VERIFY(!errors.isEmpty());
        QCOMPARE(prefix(window, owner).compare(server, Qt::CaseInsensitive), 0);
    }

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void namespaceAddressSelection_data() {
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
    void namespaceAddressSelection() {
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
        const QByteArray bytes("protected\0bytes", 15);
        QVERIFY(write(base, bytes));
        if (kind == "ads")
            QVERIFY(write(stream, QByteArray("stream bytes")));
        NativeNetworkShare share;
        if (kind == "unc") {
            const QString error = share.create();
            QVERIFY2(error.isEmpty(), qPrintable(error));
        }
        const QString requested = kind == "device" ? QStringLiteral("\\\\.\\")
                                  : kind == "ads"  ? base + ':'
                                                   : share.sharePath();
        ControlledBackend backend;
        MainWindow window(&backend);
        QStringList errors;
        QTimer responder;
        dialogs_.connectResponder(responder, &window, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(dialogs_.activeDialog())) {
                errors << box->text();
                box->accept();
            }
        });
        responder.start(5);
        window.show();
        QVERIFY(WindowActivation::waitForActive(window));
        action(window, "twoPanelsAction")->trigger();
        QVERIFY(pane(window, owner) && pane(window, 1 - owner));
        auto *address = pane(window, owner)->findChild<QLineEdit *>("archivePath");
        auto *peer = pane(window, 1 - owner)->findChild<QLineEdit *>("archivePath");
        QVERIFY(address && peer && view(window, owner));
        const QString draft = QStringLiteral("peer черновик 空");
        peer->setText(draft);
        enter(*address, requested);
        QVERIFY(!QTest::currentTestFailed());
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        if (kind == "ads") {
            QVERIFY(view(window, owner)->sourcePanel->IsAltStreamsFolder());
            bool found = false;
            for (int row = 0; row < view(window, owner)->rowCount(); ++row)
                found |= view(window, owner)->model()->index(row, 0).data().toString() ==
                         QStringLiteral("stream 空");
            QVERIFY(found);
        } else if (kind == "device")
            QVERIFY(view(window, owner)->sourcePanel->IsFSDrivesFolder());
        auto *icon = address->findChild<QAction *>("currentAddressIcon");
        QVERIFY(icon && icon->isVisible() && !icon->icon().isNull());
        inspect(window, owner, "bound");
        QVERIFY(!QTest::currentTestFailed());
        rootAndEscape(window, owner);
        QVERIFY(!QTest::currentTestFailed());
        const QString ancestor = kind == "device" ? requested
                                 : kind == "ads"  ? QDir::toNativeSeparators(fixture.path()) + '\\'
                                                  : share.server();
        selectAncestor(window, owner, kind, ancestor);
        QVERIFY(!QTest::currentTestFailed());
        inspect(window, owner, "selected");
        QVERIFY(!QTest::currentTestFailed());
        if (kind == "unc") {
            refuseRemovedShare(window, owner, share, ancestor, errors);
            QVERIFY(!QTest::currentTestFailed());
        } else
            QVERIFY(errors.isEmpty());
        QCOMPARE(peer->text(), draft);
        QCOMPARE(read(base), bytes);
        if (kind == "ads")
            QCOMPARE(read(stream), QByteArray("stream bytes"));
        if (kind == "unc")
            QCOMPARE(read(share.filePath()), NativeNetworkShare::payload());
        QCOMPARE(backend.listCalls, 0);
        QCOMPARE(backend.extractCalls, 0);
    }
};

QTEST_MAIN(NativeAddressNavigationTests)
#include "native_address_navigation_tests.moc"
