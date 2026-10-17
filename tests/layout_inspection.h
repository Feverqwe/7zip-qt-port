#pragma once

#include <QRect>
#include <QString>

class QWidget;

namespace UiInspection {
QRect bounds(const QWidget &widget, const QWidget &root);
bool fullyVisible(const QWidget &widget, const QWidget &root);
// QLineEdit paints its decorative action glyph inside a padded side widget.
bool lineEditIconVisible(const QWidget &widget, const QWidget &root);
bool separate(const QWidget &first, const QWidget &second, const QWidget &root);
bool labelFits(const QWidget &widget);
// Expected resource client after source NormalizeSize(false), including Qt minima.
QSize normalizedInitialClientSize(const QWidget &widget, const QSize &resourceClient);
// JSON omits editable values, including passwords even in show-password mode.
QString dump(const QWidget &root);
// Always writes compact JSON. PNG is opt-in through SEVENZIP_CAPTURE_SCREENSHOTS=1.
bool writeReport(QWidget &root, const QString &tag);
} // namespace UiInspection
