#include "window_state_support.h"

class PanelSelectedStateTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    static int fileRow(QTableWidget *view, const QString &path) {
        for (int row = 0; row < view->rowCount(); ++row)
            if (view->item(row, 1)->text() == path)
                return row;
        return -1;
    }
    static void createFile(const QString &path) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), 7);
    }
    static void selectRow(QTableWidget *view, int row) {
        QVERIFY(row >= 0);
        view->selectionModel()->select(view->model()->index(row, 0),
                                       QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

  private slots:
    void archiveRefreshRetainsCurrentAndSelectedItems_data() {
        filesystemRefreshRetainsCurrentAndSelectedItems_data();
    }

    void archiveRefreshRetainsCurrentAndSelectedItems() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        action(window, "flatViewAction")->setChecked(false);
        window.openArchive("selection-refresh.zip");
        const QString folder = QStringLiteral("папка с пробелом");
        const QString selected = folder + QStringLiteral("/имя.txt");
        const QString focused = folder + QStringLiteral("/текущий.txt");
        QList<ArchiveEntry> entries = {
            {selected, 1, 1}, {focused, 2, 2}, {QStringLiteral("другая/имя.txt"), 3, 3}};
        backend.completeList(entries);
        if (!flat)
            openRow(window, rowWithPath(window, folder));
        action(window, "flatViewAction")->setChecked(flat);
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        auto *view = table(window);
        view->clearSelection();
        selectRow(view, rowWithPath(window, selected));
        view->setCurrentCell(rowWithPath(window, focused), 0, QItemSelectionModel::NoUpdate);
        entries.append({folder + "/0 new.txt", 4, 4});
        action(window, "refreshAction")->trigger();
        QCOMPARE(backend.listCalls, 2);
        backend.completeList(entries);
        QCOMPARE(view->currentRow(), rowWithPath(window, focused));
        QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
        QVERIFY(view->selectionModel()->isRowSelected(rowWithPath(window, selected), {}));
        QVERIFY(!view->selectionModel()->isRowSelected(rowWithPath(window, focused), {}));
        if (!flat) {
            // Changing flat mode must preserve current separately from selection.
            action(window, "flatViewAction")->setChecked(true);
            QCOMPARE(view->currentRow(), rowWithPath(window, focused));
            action(window, "flatViewAction")->setChecked(false);
            QCOMPARE(view->currentRow(), rowWithPath(window, focused));
            QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
            // Relative names from a removed folder must not select a namesake
            // in its surviving parent after the folder fallback.
            action(window, "refreshAction")->trigger();
            backend.completeList({{QStringLiteral("имя.txt"), 5, 5}});
            QCOMPARE(view->rowCount(), 1);
            QVERIFY(view->selectionModel()->selectedRows().isEmpty());
        }
        action(window, "flatViewAction")->setChecked(false);
    }

    void failedArchiveRefreshDiscardsPendingSnapshot_data() {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("cancel") << true;
        QTest::newRow("failure") << false;
    }

    void failedArchiveRefreshDiscardsPendingSnapshot() {
        QFETCH(bool, cancel);
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        window.openArchive("first.zip");
        backend.completeList({{QStringLiteral("имя.txt"), 1, 1}});
        table(window)->selectRow(0);
        action(window, "refreshAction")->trigger();
        if (cancel)
            backend.cancel();
        else {
            QTimer::singleShot(0, &window, [] { dismissWarnings(); });
            backend.fail("fixture list failure");
        }
        QVERIFY(action(window, "createFolderAction")->isEnabled());
        window.openArchive("second.zip");
        backend.completeList({{QStringLiteral("имя.txt"), 2, 2}});
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(table(window)->selectionModel()->selectedRows().isEmpty());
    }

    void snapshotSeparatesFocusFromSelection() {
        QTableWidget view(3, 1);
        const QStringList names = {QStringLiteral("папка/z space.txt"), "a.txt", "focus.txt"};
        for (int i = 0; i < names.size(); ++i) {
            auto *item = new QTableWidgetItem(names[i]);
            item->setData(Qt::UserRole + 3, names[i].section('/', -1));
            view.setItem(i, 0, item);
        }
        CPanel panel;
        panel.AttachSelectionView(&view);
        view.setCurrentCell(2, 0, QItemSelectionModel::NoUpdate);
        selectRow(&view, 0);
        selectRow(&view, 1);
        CSelectedState state;
        panel.SaveSelectedState(state);
        QCOMPARE(state.FocusedItem, 2);
        QVERIFY(state.FocusedName_Defined);
        QCOMPARE(ToQString(state.FocusedName), names[2]);
        QVERIFY(!state.SelectFocused);
        QCOMPARE(state.SelectedNames.Size(), 2u);
        QCOMPARE(ToQString(state.SelectedNames[0]), names[1]);
        QCOMPARE(ToQString(state.SelectedNames[1]), names[0]);
        view.clearSelection();
        view.setCurrentCell(0, 0, QItemSelectionModel::NoUpdate);
        panel.RefreshListCtrl(state);
        QCOMPARE(view.currentRow(), 2);
        QVERIFY(!view.selectionModel()->isRowSelected(2, QModelIndex()));
        QVERIFY(view.selectionModel()->isRowSelected(0, QModelIndex()));
        QVERIFY(view.selectionModel()->isRowSelected(1, QModelIndex()));
        CPanel emptyPanel;
        emptyPanel.SaveSelectedState(state);
        QCOMPARE(state.FocusedItem, -1);
        QVERIFY(!state.FocusedName_Defined);
        QVERIFY(state.SelectFocused);
        QCOMPARE(state.SelectedNames.Size(), 0u);
    }

    void filesystemRefreshRetainsCurrentAndSelectedItems_data() {
        QTest::addColumn<bool>("flat");
        QTest::addColumn<int>("mode");
        for (bool flat : {false, true})
            for (int mode = 0; mode < 4; ++mode)
                QTest::newRow(qPrintable(QString("flat%1-mode%2").arg(flat).arg(mode)))
                    << flat << mode;
    }

    void filesystemRefreshRetainsCurrentAndSelectedItems() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QDir root(QFileInfo(temp.path()).canonicalFilePath());
        QVERIFY(root.mkdir(QStringLiteral("вложенная")));
        const QString prefix = flat ? QStringLiteral("вложенная/") : QString();
        const QString selected = root.filePath(prefix + QStringLiteral("a пробел.txt"));
        const QString focused = root.filePath(prefix + QStringLiteral("z текущий.txt"));
        createFile(selected);
        createFile(focused);
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(root.path());
        QTest::keyClick(address, Qt::Key_Return);
        action(window, "viewMode0Action")->trigger();
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        action(window, "flatViewAction")->setChecked(flat);
        auto *view = table(window);
        QTRY_VERIFY(fileRow(view, selected) >= 0);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        view->clearSelection();
        selectRow(view, fileRow(view, selected));
        view->setCurrentCell(fileRow(view, focused), 0, QItemSelectionModel::NoUpdate);
        createFile(root.filePath(prefix + "0 new.txt"));
        action(window, "refreshAction")->trigger();
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_VERIFY(view->currentRow() >= 0);
        QCOMPARE(view->item(view->currentRow(), 1)->text(), focused);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(view->item(view->selectionModel()->selectedRows()[0].row(), 1)->text(), selected);
        QVERIFY(!view->selectionModel()->isRowSelected(view->currentRow(), QModelIndex()));

        if (!flat) {
            action(window, "flatViewAction")->setChecked(true);
            QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
            QCOMPARE(view->item(view->currentRow(), 1)->text(), focused);
            action(window, "flatViewAction")->setChecked(false);
            QCOMPARE(view->item(view->currentRow(), 1)->text(), focused);
            QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
        }

        const int oldFocusedRow = view->currentRow();
        QVERIFY(QFile::remove(focused));
        action(window, "refreshAction")->trigger();
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_COMPARE(fileRow(view, focused), -1);
        QCOMPARE(view->currentRow(), qMin(oldFocusedRow, view->rowCount() - 1));
        QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(view->item(view->selectionModel()->selectedRows()[0].row(), 1)->text(), selected);
        action(window, "flatViewAction")->setChecked(false);
    }
};

QTEST_MAIN(PanelSelectedStateTests)
#include "panel_selected_state_tests.moc"
