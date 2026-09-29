#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/Common/Wildcard.h"
#include <QTableWidget>
#include <QtTest>

class PanelSelectionTests final : public QObject {
    Q_OBJECT

    static CObjectVector<CPanelItem> items() {
        CObjectVector<CPanelItem> result;
        for (const auto &name : {QStringLiteral("a.tar.gz"), QStringLiteral("b.GZ"),
                                 QStringLiteral("README"), QStringLiteral("folder.gz")}) {
            CPanelItem item;
            item.RelPath = name;
            item.IsFolder = name == QStringLiteral("folder.gz");
            result.Add(item);
        }
        return result;
    }

    static void populate(QTableWidget &view) {
        view.setColumnCount(1);
        view.setRowCount(5);
        view.setSelectionBehavior(QAbstractItemView::SelectRows);
        view.setSelectionMode(QAbstractItemView::ExtendedSelection);
        for (int row = 0; row < 5; ++row) {
            auto *item = new QTableWidgetItem(row == 0 ? QStringLiteral("..")
                                                       : items()[unsigned(row - 1)].RelPath);
            item->setData(Qt::UserRole + 6, row == 0 ? quint64(UInt32(-1)) : quint64(row - 1));
            view.setItem(row, 0, item);
        }
    }

    static bool selected(const QTableWidget &view, int row) {
        return view.selectionModel()->isRowSelected(row, QModelIndex());
    }

  private slots:
    void upstreamWildcardRules() {
        QVERIFY(DoesWildcardMatchName(u"", u""));
        QVERIFY(!DoesWildcardMatchName(u"", u"a"));
        QVERIFY(DoesWildcardMatchName(u"***", u""));
        QVERIFY(!DoesWildcardMatchName(u"?", u""));
        QVERIFY(DoesWildcardMatchName(u"?", u"я"));
        QVERIFY(!DoesWildcardMatchName(u"?", u"ab"));
        QVERIFY(DoesWildcardMatchName(u"a*b?c", u"a one b two bxc"));
        QVERIFY(!DoesWildcardMatchName(u"*.txt", u"file.txt.bak"));
        QVERIFY(!DoesWildcardMatchName(u"*.*", u"README"));
        QVERIFY(DoesWildcardMatchName(u"АЛЬФА *.TXT", u"альфа файл.txt"));
        QVERIFY(!DoesWildcardMatchName(u"?", u"\U0001F600"));
        QVERIFY(DoesWildcardMatchName(u"??", u"\U0001F600"));
        QVERIFY(DoesWildcardMatchName(UString(L"*.gz"), UString(L"b.GZ")));
    }

    void sourcePanelCommandsKeepIndexAndFocus() {
        CPanel panel(QStringLiteral("/tmp/"), items());
        QTableWidget view;
        populate(view);
        panel.AttachSelectionView(&view);
        CSelectedState snapshot;
        panel.SaveSelectedState(snapshot);

        view.selectionModel()->setCurrentIndex(view.model()->index(1, 0),
                                               QItemSelectionModel::NoUpdate);
        panel.SelectByType(true);
        QVERIFY(selected(view, 1));
        QVERIFY(selected(view, 2));
        QVERIFY(!selected(view, 3));
        QVERIFY(!selected(view, 4));
        QVERIFY(!selected(view, 0));
        QCOMPARE(view.currentRow(), 1);

        panel.SelectSpec(UString(L"README"), true);
        QVERIFY(selected(view, 3));
        panel.SelectAll(false);
        QCOMPARE(view.selectionModel()->selectedRows().size(), 0);
        panel.InvertSelection();
        QCOMPARE(view.selectionModel()->selectedRows().size(), 4);
        QVERIFY(!selected(view, 0));
        panel.SelectByType(false);
        QVERIFY(!selected(view, 1));
        QVERIFY(!selected(view, 2));
        QVERIFY(selected(view, 3));
        QVERIFY(selected(view, 4));

        view.selectionModel()->setCurrentIndex(view.model()->index(0, 0),
                                               QItemSelectionModel::NoUpdate);
        panel.SelectByType(false);
        QVERIFY(selected(view, 4));
    }
};
QTEST_MAIN(PanelSelectionTests)
#include "panel_selection_tests.moc"
