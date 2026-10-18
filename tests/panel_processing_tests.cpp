#include "qtpanelfolder.h"
#include <QTableWidget>
#include <QtTest>

class PanelProcessingTests : public QObject {
    Q_OBJECT

  private slots:
    void providerlessAttachmentDoesNotInventMarks() {
        CPanel panel;
        QTableWidget table(2, 1);
        table.setSelectionBehavior(QAbstractItemView::SelectRows);
        for (int row = 0; row < 2; ++row) {
            auto *item = new QTableWidgetItem(QString::number(row));
            item->setData(Qt::UserRole + 6, UInt32(row));
            table.setItem(row, 0, item);
        }
        table.selectRow(1);
        panel.AttachSelectionView(&table);
        CRecordVector<UInt32> selected;
        panel.Get_ItemIndices_Selected(selected);
        QVERIFY(selected.IsEmpty());
        table.selectRow(0);
        panel.Get_ItemIndices_Selected(selected);
        QVERIFY(selected.IsEmpty());
        CObjectVector<CPanelItem> items;
        for (int index = 0; index < 2; ++index) {
            CPanelItem item;
            item.RelPath = QStringLiteral("owned 文件 %1.txt").arg(index);
            items.Add(item);
        }
        panel.SetNewFolder(new QtPanelFolder(QStringLiteral("/fixture/"), items));
        QCOMPARE(panel.LoadListItems(), HRESULT(S_OK));
        table.setRowCount(2);
        for (int row = 0; row < 2; ++row) {
            auto *item = new QTableWidgetItem(items[unsigned(row)].RelPath);
            item->setData(Qt::UserRole + 6, UInt32(row));
            table.setItem(row, 0, item);
        }
        // The existing attachment must keep its live selection callback.
        table.selectRow(1);
        panel.Get_ItemIndices_Selected(selected);
        QCOMPARE(selected.Size(), 1u);
        QCOMPARE(selected[0], 1u);
        panel.ReleaseFolder();
        QCOMPARE(table.rowCount(), 0);
        panel.Get_ItemIndices_Selected(selected);
        QVERIFY(selected.IsEmpty());
    }
    void absentFolderRefreshPreservesPresentation_data() {
        QTest::addColumn<bool>("released");
        QTest::addColumn<bool>("populated");
        QTest::addColumn<bool>("disabled");
        QTest::addColumn<bool>("failedLoad");
        for (bool released : {false, true})
            for (bool populated : {false, true})
                for (bool disabled : {false, true})
                    for (bool failed : {false, true})
                        QTest::newRow(qPrintable(QString("%1-%2-%3-%4")
                                                     .arg(released)
                                                     .arg(populated)
                                                     .arg(disabled)
                                                     .arg(failed)))
                            << released << populated << disabled << failed;
    }

    void absentFolderRefreshPreservesPresentation() {
        QFETCH(bool, released);
        QFETCH(bool, populated);
        QFETCH(bool, disabled);
        QFETCH(bool, failedLoad);
        CObjectVector<CPanelItem> items;
        CPanelItem entry;
        entry.RelPath = QStringLiteral("control 文件.txt");
        items.Add(entry);
        CPanel panel;
        QTableWidget table(0, 1);
        table.setSelectionBehavior(QAbstractItemView::SelectRows);
        panel.AttachSelectionView(&table);
        if (released) {
            panel.SetNewFolder(new QtPanelFolder(QStringLiteral("/fixture/"), items));
            panel.ReleaseFolder();
        }
        QVERIFY(!panel.GetFolder());
        // A queued restoration can outlive provider teardown. Keep a parent row
        // in the attached view to prove it does not become a provider substitute.
        if (populated) {
            table.setRowCount(1);
            auto *parent = new QTableWidgetItem("..");
            parent->setData(Qt::UserRole + 6, UInt32(-1));
            table.setItem(0, 0, parent);
            table.selectRow(0);
        }
        if (disabled)
            panel.Disable_Processing_Timer_Notify_StatusBar();
        if (failedLoad)
            QCOMPARE(panel.LoadListItems(true, E_ABORT), HRESULT(E_ABORT));
        int statusCalls = 0;
        panel.AttachStatusPresentation([&](const CPanelStatus &) { ++statusCalls; });
        QSignalSpy selection(table.selectionModel(), &QItemSelectionModel::selectionChanged);
        QSignalSpy focus(&table, &QTableWidget::currentCellChanged);
        CSelectedState saved;
        saved.FocusedName = L"unused 文件.txt";
        saved.FocusedName_Defined = true;
        QCOMPARE(panel.RefreshListCtrl(saved), HRESULT(S_OK));
        QCOMPARE(table.rowCount(), populated ? 1 : 0);
        QCOMPARE(table.selectionModel()->selectedRows().size(), populated ? 1 : 0);
        QCOMPARE(table.currentRow(), populated ? 0 : -1);
        QCOMPARE(selection.count(), 0);
        QCOMPARE(focus.count(), 0);
        QCOMPARE(statusCalls, 0);
        QCOMPARE(panel.ShouldProcessNotifications(), !disabled);
        QVERIFY(!panel.TakeRefreshSelection());
        // The ordinary snapshot transport has a provider and must still restore.
        panel.SetNewFolder(new QtPanelFolder(QStringLiteral("/fixture/"), items));
        QCOMPARE(panel.LoadListItems(), HRESULT(S_OK));
        table.setRowCount(1);
        auto *item = new QTableWidgetItem(entry.RelPath);
        item->setData(Qt::UserRole + 6, 0u);
        table.setItem(0, 0, item);
        saved.FocusedName = ToUString(entry.RelPath);
        saved.SelectedNames.Add(saved.FocusedName);
        QCOMPARE(panel.RefreshListCtrl(saved), HRESULT(S_OK));
        QCOMPARE(table.selectionModel()->selectedRows().size(), 1);
        QCOMPARE(statusCalls, 1);
        QVERIFY(panel.ShouldProcessNotifications());
    }

    void nestedNotifyGuardsRestoreAfterException() {
        CPanel panel;
        QVERIFY(panel.ShouldProcessNotifications());
        {
            CPanel::CDisableNotify outer(panel);
            QVERIFY(!panel.ShouldProcessNotifications());
            try {
                CPanel::CDisableNotify inner(panel);
                QVERIFY(!panel.ShouldProcessNotifications());
                throw 42;
            } catch (int) {
            }
            QVERIFY(!panel.ShouldProcessNotifications());
        }
        QVERIFY(panel.ShouldProcessNotifications());
    }

    void disabledPanelStaysDisabledUnlessSourceMemModeEnablesRestore() {
        CPanel panel;
        panel.Disable_Processing_Timer_Notify_StatusBar();
        QVERIFY(!panel.ShouldProcessNotifications());
        {
            CPanel::CDisableTimerProcessing timer(panel);
            CPanel::CDisableTimerProcessing2 timer2(&panel);
            timer2.Restore();
            timer2.Restore();
            CPanel::CDisableTimerProcessing2 absent(NULL);
            absent.Restore();
            CPanel::CDisableNotify notify(panel);
        }
        QVERIFY(!panel.ShouldProcessNotifications());
        {
            CPanel::CDisableNotify notify(panel);
            notify.SetMemMode_Enable();
            QVERIFY(!panel.ShouldProcessNotifications());
        }
        QVERIFY(panel.ShouldProcessNotifications());
    }

    void sourceRestoreSuppressesIntermediateConsumersAndEnablesFinalFocus() {
        CObjectVector<CPanelItem> items;
        for (const QString &name :
             {QStringLiteral("первый й.txt"), QStringLiteral("второй й.txt")}) {
            CPanelItem item;
            item.RelPath = name;
            items.Add(item);
        }
        CPanel panel(QStringLiteral("/folder with spaces/"), items);
        QTableWidget table(2, 1);
        table.setSelectionBehavior(QAbstractItemView::SelectRows);
        for (unsigned index = 0; index < items.Size(); ++index) {
            auto *item = new QTableWidgetItem(items[index].RelPath);
            item->setData(Qt::UserRole + 3, items[index].RelPath);
            item->setData(Qt::UserRole + 6, index);
            table.setItem(int(index), 0, item);
        }
        panel.AttachSelectionView(&table);
        table.selectRow(0);
        CSelectedState saved;
        panel.SaveSelectedState(saved);
        table.selectRow(1);
        QSignalSpy raw(table.selectionModel(), &QItemSelectionModel::selectionChanged);
        int consumerCalls = 0;
        int focusConsumerCalls = 0;
        connect(table.selectionModel(), &QItemSelectionModel::selectionChanged, &table, [&] {
            if (panel.ShouldProcessNotifications())
                ++consumerCalls;
        });
        connect(&table, &QTableWidget::currentCellChanged, &table, [&] {
            if (panel.ShouldProcessNotifications())
                ++focusConsumerCalls;
        });
        panel.RefreshListCtrl(saved);
        QVERIFY(raw.count() > 0);
        QCOMPARE(consumerCalls, 0);
        QCOMPARE(focusConsumerCalls, 1);
        QVERIFY(panel.ShouldProcessNotifications());
        QCOMPARE(table.currentRow(), 0);
        QCOMPARE(table.selectionModel()->selectedRows().size(), 1);
        QVERIFY(table.selectionModel()->isRowSelected(0, {}));
        CRecordVector<UInt32> selected;
        panel.Get_ItemIndices_Selected(selected);
        QCOMPARE(selected.Size(), 1u);
        QCOMPARE(selected[0], UInt32(0));
        table.selectRow(1);
        QVERIFY(consumerCalls > 0);
        panel.Disable_Processing_Timer_Notify_StatusBar();
        consumerCalls = 0;
        focusConsumerCalls = 0;
        panel.RefreshListCtrl(saved);
        QCOMPARE(consumerCalls, 0);
        QCOMPARE(focusConsumerCalls, 1);
        QVERIFY(panel.ShouldProcessNotifications());
    }
};

QTEST_MAIN(PanelProcessingTests)
#include "panel_processing_tests.moc"
