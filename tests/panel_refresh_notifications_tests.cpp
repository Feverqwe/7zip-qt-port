#include "filetable.h"
#include "panel_load_folder.h"
#include <QtTest>

class PanelRefreshNotificationsTests final : public QObject {
    Q_OBJECT
  private slots:
    void focusRequestsSourceName_data() {
        QTest::addColumn<bool>("fast");
        QTest::addColumn<bool>("fallback");
        QTest::addColumn<bool>("alternative");
        for (bool fast : {false, true})
            for (bool fallback : {false, true})
                for (bool alternative : {false, true})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(fast).arg(fallback).arg(alternative)))
                        << fast << fallback << alternative;
    }
    void focusRequestsSourceName() {
        QFETCH(bool, fast);
        QFETCH(bool, fallback);
        QFETCH(bool, alternative);
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        spec->fastNames = fast;
        spec->firstName = L"first     文件";
        spec->secondName = L"second δ.txt";
        CPanel panel(folder);
        QCOMPARE(panel.LoadListItems(), S_OK);
        FileTable view(2, 1);
        view.sourcePanel = &panel;
        view.textFolder = folder;
        view.propertyIds = {kpidName};
        view.setSelectionBehavior(QAbstractItemView::SelectRows);
        for (int row = 0; row < 2; ++row) {
            auto *item = new ArchiveTableItem("uncached display");
            item->setData(Qt::UserRole + 6, 1 - row);
            view.setItem(row, 0, item);
        }
        panel.AttachSelectionView(&view);
        panel.SetAlternativeSelection(alternative);
        const int target = fallback ? 1 : 0;
        view.setCurrentCell(1 - target, 0, QItemSelectionModel::NoUpdate);
        // Leave the target uncached, as on the first callback after publication.
        view.item(target, 0)->setData(Qt::UserRole + 6, 1 - target);
        CSelectedState state;
        state.FocusedName = fallback ? UString(L"missing.txt") : spec->secondName;
        state.FocusedName_Defined = true;
        state.FocusedItem = target;
        state.SelectFocused = true;
        state.SelectedNames.Add(spec->secondName);
        int requests = 0;
        bool callbacksEnabled = false;
        QString displayed;
        connect(&view, &QTableWidget::currentCellChanged, &view, [&](int row) {
            ++requests;
            callbacksEnabled = panel.ShouldProcessNotifications();
            displayed = view.item(row, 0)->text();
        });
        QCOMPARE(panel.RefreshListCtrl(state), S_OK);
        QCOMPARE(requests, 1);
        QVERIFY(callbacksEnabled);
        QCOMPARE(displayed, fallback ? (fast ? QStringLiteral("first ... 文件")
                                             : QStringLiteral("first     文件"))
                                     : QStringLiteral("second δ.txt"));
        QCOMPARE(view.currentRow(), target);
        QVERIFY(panel.IsItemMarked(1));
        QVERIFY(panel.ShouldProcessNotifications());
    }
    void failedLoadKeepsOuterGuard_data() {
        QTest::addColumn<HRESULT>("result");
        QTest::newRow("failure") << E_FAIL;
        QTest::newRow("cancel") << E_ABORT;
    }
    void failedLoadKeepsOuterGuard() {
        QFETCH(HRESULT, result);
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        CPanel panel(folder);
        QTableWidget view(1, 1);
        view.setItem(0, 0, new QTableWidgetItem("existing row"));
        panel.AttachSelectionView(&view);
        QSignalSpy focus(&view, &QTableWidget::currentCellChanged);
        {
            CPanel::CDisableNotify outer(panel);
            QVERIFY(!panel.ShouldProcessNotifications());
            QCOMPARE(panel.LoadListItems(true, result), result);
            QCOMPARE(panel.RefreshListCtrl(), result);
            QCOMPARE(focus.count(), 0);
            QVERIFY(!panel.ShouldProcessNotifications());
        }
        QVERIFY(panel.ShouldProcessNotifications());
    }
};

QTEST_MAIN(PanelRefreshNotificationsTests)
#include "panel_refresh_notifications_tests.moc"
