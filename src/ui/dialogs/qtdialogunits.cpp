#include "ui/dialogs/qtdialogunits.h"
#include <QFontMetricsF>
#include <cmath>

QtDialogUnits::QtDialogUnits(const QFont &font) {
    const QFontMetricsF metrics(font);
    _horizontal = metrics.horizontalAdvance(
                      QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")) /
                  52 / 4;
    // QLabel rounds line height upward; mapped controls must fit that height.
    _vertical = std::ceil(metrics.height()) / 8;
}

QSize QtDialogUnits::pixels(const QSize &units) const {
    return {int(std::round(units.width() * _horizontal)),
            int(std::round(units.height() * _vertical))};
}
