#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "external_editor_fixture.h"
#include "layout_inspection.h"
#include "window_state_support.h"

class AddressComboTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static void selectPopupRow(QComboBox &combo, int index) {
        auto *view = combo.view();
        view->scrollTo(view->model()->index(index, 0));
        QApplication::processEvents();
        QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                          view->visualRect(view->model()->index(index, 0)).center());
    }

  private slots:
    void init() {
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QVERIFY(!settings.isEmpty());
        QFile::remove(settings);
        ReloadLang(settings);
    }
    void dropdownContainsCurrentHierarchy_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const char *language : {"-", "ru"})
            for (const QSize size : {QSize(980, 620), QSize(640, 400)})
                QTest::newRow(qPrintable(QString("%1-%2").arg(language).arg(size.width())))
                    << QString::fromLatin1(language) << size;
    }
    void dropdownContainsCurrentHierarchy() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString parent = fixture.filePath(QStringLiteral("родитель space"));
        const QString child = parent + QStringLiteral("/внутри 密碼");
        QVERIFY(QDir().mkpath(child));
        QFile payload(child + "/keep.txt");
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QCOMPARE(payload.write("unchanged"), 9);
        payload.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.resize(size);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        QVERIFY(address);
        address->setText(child);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        auto *combo = window.findChild<QComboBox *>("pathCombo");
        QVERIFY(combo);
        combo->showPopup();
        QApplication::processEvents();
        QVERIFY2(combo->findText(QStringLiteral("родитель space")) >= 0,
                 "The source dropdown must include the current path's parent component.");
        QVERIFY(combo->findText(QStringLiteral("внутри 密碼")) >= 0);
        QVERIFY(combo->findText(QString::fromWCharArray(LangString(IDS_COMPUTER))) >= 0);
        QCOMPARE(combo->currentText(), address->text());
        QVERIFY(UiInspection::fullyVisible(*combo, window));
        QVERIFY(UiInspection::fullyVisible(*table(window), window));
        QVERIFY(UiInspection::writeReport(
            window, QString("address-combo-%1-%2").arg(language).arg(size.width())));
        const int row = combo->findText(QStringLiteral("родитель space"));
        selectPopupRow(*combo, row);
        QTRY_COMPARE(QDir::cleanPath(address->text()), QFileInfo(parent).canonicalFilePath());
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("внутри 密碼"));
        combo->showPopup();
        QApplication::processEvents();
        const int computer = combo->findText(QString::fromWCharArray(LangString(IDS_COMPUTER)));
        QVERIFY(computer >= 0);
        selectPopupRow(*combo, computer);
#ifdef Q_OS_WIN
        QVERIFY(!action(window, "createFolderAction")->isEnabled());
#else
        QTRY_COMPARE(address->text(), QStringLiteral("/"));
#endif
        QVERIFY(payload.open(QIODevice::ReadOnly));
        QCOMPARE(payload.readAll(), QByteArray("unchanged"));
        QCOMPARE(backend.listCalls, 0);
    }
    void archiveHierarchySelectionPreservesArchive_data() {
        QTest::addColumn<bool>("nested");
        QTest::newRow("single") << false;
        QTest::newRow("nested") << true;
    }
    void archiveHierarchySelectionPreservesArchive() {
        QFETCH(bool, nested);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString entry = QStringLiteral("внешняя space/内側 密碼/keep.txt");
        QVERIFY(QDir().mkpath(QFileInfo(fixture.filePath(entry)).absolutePath()));
        QVERIFY(ExternalEditorFixture::write(fixture.filePath(entry), "unchanged"));
        const QString archive = fixture.filePath(QStringLiteral("архив δ.zip"));
        QVERIFY(ExternalEditorFixture::createArchive(fixture.path(), archive, entry));
        const QByteArray original = ExternalEditorFixture::read(archive);
        const QString outer = fixture.filePath(QStringLiteral("outer space.zip"));
        QByteArray originalOuter;
        if (nested) {
            QVERIFY(ExternalEditorFixture::createArchive(fixture.path(), outer,
                                                         QFileInfo(archive).fileName()));
            originalOuter = ExternalEditorFixture::read(outer);
        }
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        window.openArchive(nested ? outer : archive);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *combo = window.findChild<QComboBox *>("pathCombo");
        QVERIFY(address && combo);
        QTRY_VERIFY(action(window, "parentFolderAction")->isEnabled());
        if (nested) {
            QTRY_COMPARE(table(window)->rowCount(), 1);
            QTRY_COMPARE(table(window)->item(0, 0)->text(), QFileInfo(archive).fileName());
            openRow(window, 0);
            QTRY_COMPARE(table(window)->item(0, 0)->text(), QStringLiteral("внешняя space"));
        }
        const QString archiveAddress = address->text();
        address->setText(archiveAddress + QStringLiteral(" / внешняя space/内側 密碼"));
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("keep.txt"));
        combo->showPopup();
        QApplication::processEvents();
        QVERIFY(combo->findText(QStringLiteral("архив δ.zip")) >= 0);
        const int parent = combo->findText(QStringLiteral("внешняя space"));
        QVERIFY(parent >= 0);
        selectPopupRow(*combo, parent);
        QTRY_COMPARE(address->text(), archiveAddress + QStringLiteral(" / внешняя space"));
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("内側 密碼"));
        QCOMPARE(ExternalEditorFixture::read(archive), original);
        if (nested)
            QCOMPARE(ExternalEditorFixture::read(outer), originalOuter);
        QCOMPARE(ExternalEditorFixture::read(fixture.filePath(entry)), QByteArray("unchanged"));
    }
    void eachPanelUsesItsOwnHierarchyAndEscapeRestoresAddress() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString leftPath = fixture.filePath(QStringLiteral("left space/левый"));
        const QString rightPath = fixture.filePath(QStringLiteral("right space/右"));
        QVERIFY(QDir().mkpath(leftPath));
        QVERIFY(QDir().mkpath(rightPath));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        QWidget *panels[] = {window.findChild<QWidget *>("filePanel0"),
                             window.findChild<QWidget *>("filePanel1")};
        const QString paths[] = {leftPath, rightPath};
        for (unsigned i = 0; i < 2; ++i) {
            QVERIFY(panels[i]);
            auto *address = panels[i]->findChild<QLineEdit *>("archivePath");
            auto *view = panels[i]->findChild<QTableWidget *>("archiveTable");
            address->setFocus();
            QTRY_VERIFY(address->hasFocus());
            action(window, "viewMode3Action")->trigger();
            address->setText(paths[i]);
            QTest::keyClick(address, Qt::Key_Return);
            QTRY_VERIFY(view->isEnabled());
            QTRY_COMPARE(QDir::cleanPath(address->text()), QFileInfo(paths[i]).canonicalFilePath());
            QTRY_VERIFY2(view->hasFocus(),
                         qPrintable(QString("panel=%1 visible=%2 focus=%3")
                                        .arg(i)
                                        .arg(view->isVisible())
                                        .arg(QApplication::focusWidget()
                                                 ? QApplication::focusWidget()->objectName()
                                                 : QStringLiteral("none"))));
        }
        for (unsigned i = 0; i < 2; ++i) {
            auto *combo = panels[i]->findChild<QComboBox *>("pathCombo");
            auto *address = panels[i]->findChild<QLineEdit *>("archivePath");
            address->setFocus();
            QTRY_VERIFY(address->hasFocus());
            combo->showPopup();
            QApplication::processEvents();
            QStringList rows;
            for (int row = 0; row < combo->count(); ++row)
                rows.append(combo->itemText(row));
            QVERIFY2(combo->findText(i == 0 ? QStringLiteral("левый") : QStringLiteral("右")) >= 0,
                     qPrintable(QString("panel=%1 address=%2 rows=%3")
                                    .arg(i)
                                    .arg(address->text(), rows.join(" | "))));
            QVERIFY(combo->findText(i == 0 ? QStringLiteral("右") : QStringLiteral("левый")) < 0);
            combo->hidePopup();
            window.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&window));
            address->setFocus();
            QTRY_VERIFY(address->hasFocus());
            address->setText(QStringLiteral("uncommitted address"));
            QTest::keyClick(address, Qt::Key_Escape);
            QCOMPARE(QDir::cleanPath(address->text()), QFileInfo(paths[i]).canonicalFilePath());
            QTRY_VERIFY(panels[i]->findChild<QTableWidget *>("archiveTable")->hasFocus());
        }
        QCOMPARE(backend.listCalls, 0);
    }
};

QTEST_MAIN(AddressComboTests)
#include "address_combo_tests.moc"
