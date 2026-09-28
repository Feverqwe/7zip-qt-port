#pragma once

#include <QRect>
#include <QString>

class QWidget;

namespace UiInspection {
QRect bounds(const QWidget &widget, const QWidget &root);
bool fullyVisible(const QWidget &widget, const QWidget &root);
bool separate(const QWidget &first, const QWidget &second, const QWidget &root);
bool labelFits(const QWidget &widget);
// JSON omits editable values, including passwords even in show-password mode.
QString dump(const QWidget &root);
// Always writes JSON. PNG is opt-in through SEVENZIP_CAPTURE_SCREENSHOTS=1.
bool writeReport(QWidget &root, const QString &tag);
} // namespace UiInspection
