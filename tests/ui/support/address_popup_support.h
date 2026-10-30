#pragma once

#include <QAbstractItemView>
#include <QComboBox>
#include <QtTest>

namespace AddressPopupSupport {
inline void show(QComboBox &combo) { combo.showPopup(); }

inline void selectRow(QComboBox &combo, int row) {
    auto *view = combo.view();
    const QModelIndex index = view->model()->index(row, 0);
    QVERIFY(index.isValid());
    QVERIFY(QTest::qWaitForWindowExposed(view->window()));
    view->scrollTo(index);
    QTRY_VERIFY(view->viewport()->rect().contains(view->visualRect(index).center()));
    const QPoint point = view->visualRect(index).center();
    auto *popup = view->window();
    QTest::mouseMove(popup->windowHandle(), view->viewport()->mapTo(popup, QPoint(1, 1)));
    QTest::mouseMove(popup->windowHandle(), view->viewport()->mapTo(popup, point));
    QCOMPARE(view->indexAt(point), index);
    QSignalSpy activated(&combo, &QComboBox::activated);
    QTest::mousePress(popup->windowHandle(), Qt::LeftButton, Qt::NoModifier,
                      view->viewport()->mapTo(popup, point));
    QTRY_COMPARE(view->currentIndex(), index);
    QTest::mouseRelease(popup->windowHandle(), Qt::LeftButton, Qt::NoModifier,
                        view->viewport()->mapTo(popup, point));
    QTRY_COMPARE(activated.size(), 1);
    QCOMPARE(activated.front().front().toInt(), row);
    QTRY_VERIFY(!view->isVisible());
}
} // namespace AddressPopupSupport
