#include "window_state_support.h"

class PanelKeyTests final : public QObject {
    Q_OBJECT

    static QAction *action(MainWindow &window, const char *name) {
        return window.findChild<QAction *>(name);
    }

  private slots:
    void addressKeys_data() { keypadSelection_data(); }
    void addressKeys() {
        QFETCH(int, mode);
        QFETCH(bool, archive);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(dir.path());
        QTest::keyClick(address, Qt::Key_Return);
        if (archive) {
            window.openArchive(dir.filePath("адрес space.zip"));
            backend.completeList({ArchiveEntry{"папка/файл.txt", 1, 1}});
        }
        action(window, qPrintable(QStringLiteral("viewMode%1Action").arg(mode)))->trigger();
        window.show();
        window.activateWindow();
        QApplication::processEvents();
        auto *combo = window.findChild<QComboBox *>("pathCombo");
        QWidget *view =
            mode == 3 ? static_cast<QWidget *>(window.findChild<QTableWidget *>("archiveTable"))
                      : static_cast<QWidget *>(window.findChild<QListView *>("iconList"));
        const QString currentPath = address->text();
        for (int key : {Qt::Key_F1, Qt::Key_F2}) {
            view->setFocus();
            QTest::keyClick(view, static_cast<Qt::Key>(key), Qt::AltModifier);
            QVERIFY(combo->view()->isVisible());
            QCOMPARE(address->text(), currentPath);
            combo->hidePopup();
            address->setFocus();
            QTest::keyClick(address, static_cast<Qt::Key>(key), Qt::AltModifier);
            QVERIFY(combo->view()->isVisible());
            combo->hidePopup();
            address->setFocus();
            QTest::keyClick(address, Qt::Key_Tab);
            QTRY_VERIFY(view->hasFocus());
            QCOMPARE(address->text(), currentPath);
            QTest::keyClick(view, static_cast<Qt::Key>(key), Qt::AltModifier | Qt::ShiftModifier);
            QVERIFY(!combo->view()->isVisible());
        }
        if (mode == 3 && !archive) {
            const QString screenshots = QDir::currentPath() + "/ui-screenshots";
            QVERIFY(QDir().mkpath(screenshots));
            for (int width : {980, 640}) {
                combo->hidePopup();
                window.resize(width, width == 980 ? 620 : 400);
                QCOMPARE(window.width(), width);
                combo->showPopup();
                QApplication::processEvents();
                QVERIFY(window.grab().save(screenshots +
                                           QStringLiteral("/address-key-%1.png").arg(width)));
                QVERIFY(combo->view()->window()->grab().save(
                    screenshots + QStringLiteral("/address-popup-%1.png").arg(width)));
            }
        }
    }

    void createFileKey_data() {
        QTest::addColumn<int>("mode");
        for (int mode = 0; mode < 4; ++mode)
            QTest::newRow(qPrintable(QString::number(mode))) << mode;
    }
    void createFileKey() {
        QFETCH(int, mode);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(dir.path());
        QTest::keyClick(address, Qt::Key_Return);
        action(window, qPrintable(QStringLiteral("viewMode%1Action").arg(mode)))->trigger();
        window.show();
        QWidget *view =
            mode == 3 ? static_cast<QWidget *>(window.findChild<QTableWidget *>("archiveTable"))
                      : static_cast<QWidget *>(window.findChild<QListView *>("iconList"));
        int dialogs = 0;
        const QString name = QStringLiteral("новый файл.txt");
        const auto pressCreate = [&](QWidget *target, bool accept) {
            QTimer::singleShot(0, &window, [&window, &dialogs, &name, accept] {
                if (auto *dialog = window.findChild<QDialog *>("comboDialog")) {
                    ++dialogs;
                    dialog->findChild<QComboBox *>("comboValue")->setEditText(name);
                    if (accept)
                        dialog->accept();
                    else
                        dialog->reject();
                }
            });
            target->setFocus();
            QTest::keyClick(target, Qt::Key_F4, Qt::ShiftModifier);
            QApplication::processEvents();
        };
        pressCreate(view, false);
        QCOMPARE(dialogs, 1);
        QVERIFY(!QFileInfo::exists(dir.filePath(name)));
        pressCreate(address, true);
        QCOMPARE(dialogs, 1);
        pressCreate(view, true);
        QCOMPARE(dialogs, 2);
        QVERIFY(QFileInfo::exists(dir.filePath(name)));
        QCOMPARE(QFileInfo(dir.filePath(name)).size(), qint64(0));
        window.openArchive(dir.filePath("busy.zip"));
        pressCreate(view, true);
        QCOMPARE(dialogs, 2);
        backend.completeList({ArchiveEntry{"file.txt", 1, 1}});
        pressCreate(view, true);
        QCOMPARE(dialogs, 2);
        QCOMPARE(backend.extractCalls, 0);
    }

    void keypadSelection_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("archive");
        for (int mode = 0; mode < 4; ++mode) {
            QTest::newRow(qPrintable(QStringLiteral("archive-%1").arg(mode))) << mode << true;
            QTest::newRow(qPrintable(QStringLiteral("disk-%1").arg(mode))) << mode << false;
        }
    }

    void keypadSelection() {
        QFETCH(int, mode);
        QFETCH(bool, archive);
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.remove("view/flatDisk");
        settings.remove("view/flatArchive");
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        const QStringList names = {QStringLiteral("Альфа файл.txt"), "beta.TXT", "other.bin"};
        if (archive) {
            window.openArchive("keys.zip");
            backend.completeList({ArchiveEntry{names[0], 1, 1}, ArchiveEntry{names[1], 2, 2},
                                  ArchiveEntry{names[2], 3, 3}});
        } else {
            for (const auto &name : names) {
                QFile file(temp.filePath(name));
                QVERIFY(file.open(QIODevice::WriteOnly));
            }
            auto *address = window.findChild<QLineEdit *>("archivePath");
            address->setText(temp.path());
            QTest::keyClick(address, Qt::Key_Return);
        }
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        action(window, qPrintable(QStringLiteral("viewMode%1Action").arg(mode)))->trigger();
        window.show();
        QWidget *view = mode == 3
                            ? static_cast<QWidget *>(table)
                            : static_cast<QWidget *>(window.findChild<QListView *>("iconList"));
        QVERIFY(view);
        view->setFocus();
        for (int row = 0; row < table->rowCount(); ++row)
            if (table->item(row, 0)->text() == names[0])
                table->setCurrentCell(row, 0);
        table->clearSelection();
        const auto keypad = Qt::KeypadModifier;
        QTest::keyClick(view, Qt::Key_Plus, keypad | Qt::ShiftModifier);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 3);
        QTest::keyClick(view, Qt::Key_Minus, keypad | Qt::ShiftModifier);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 0);
        QTest::keyClick(view, Qt::Key_Plus, keypad | Qt::AltModifier);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 2);
        QTest::keyClick(view, Qt::Key_Minus, keypad | Qt::AltModifier);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 0);
        QTest::keyClick(view, Qt::Key_Asterisk, keypad);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 3);
        QTest::keyClick(view, Qt::Key_Asterisk, keypad);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 0);

        int dialogs = 0;
        const auto maskKey = [&](int key) {
            QTimer::singleShot(0, &window, [&] {
                for (QWidget *widget : QApplication::topLevelWidgets())
                    if (auto *dialog = dynamic_cast<ComboDialog *>(widget);
                        dialog && dialog->isVisible()) {
                        ++dialogs;
                        dialog->findChild<QLineEdit *>()->setText("*.txt");
                        dialog->accept();
                    }
            });
            QTest::keyClick(view, static_cast<Qt::Key>(key), keypad);
        };
        maskKey(Qt::Key_Plus);
        QCOMPARE(dialogs, 1);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 2);
        maskKey(Qt::Key_Minus);
        QCOMPARE(dialogs, 2);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 0);

        QTest::keyClick(view, Qt::Key_Asterisk);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 0);
        QTest::keyClick(view, Qt::Key_Plus, keypad | Qt::ControlModifier);
        QCOMPARE(dialogs, 2);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 0);

        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setFocus();
        QTest::keyClick(address, Qt::Key_Asterisk, keypad);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 0);
        window.openArchive("busy.zip");
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Plus, keypad | Qt::ShiftModifier);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 0);
        backend.cancel();
    }
};

QTEST_MAIN(PanelKeyTests)
#include "panel_key_tests.moc"
