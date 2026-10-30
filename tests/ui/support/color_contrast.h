#pragma once
#include <QColor>
#include <cmath>

inline double textContrast(const QColor &foreground, const QColor &background) {
    const auto luminance = [](const QColor &color) {
        const auto linear = [](double value) {
            return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) +
               0.0722 * linear(color.blueF());
    };
    const double first = luminance(foreground), second = luminance(background);
    return (qMax(first, second) + 0.05) / (qMin(first, second) + 0.05);
}
