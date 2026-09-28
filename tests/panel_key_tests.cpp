#include "window_state_support.h"

class PanelKeyTests final : public QObject {
    Q_OBJECT

    static QAction *action(MainWindow &window, const char *name) {
        return window.findChild<QAction *>(name);
    }

  private slots:
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
