#pragma once

#include <QPointF>
#include <QtGlobal>

// Qt 6.12 accepts fractional drag positions; older kits require QPoint.
inline auto DragEventPosition(const QPoint &point) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)
    return QPointF(point);
#else
    return point;
#endif
}
