#include "qtpanelfolder.h"
#include <QTableWidget>
#include <QtTest>

class PanelProcessingTests : public QObject {
    Q_OBJECT

  private slots:
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
            } catch (int value) {
                QCOMPARE(value, 42);
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
