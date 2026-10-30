// Lucide geometry adapted to QPainterPath from icons/{plus,folder-output,check,
// copy,move-right,trash,info,arrow-up,folder-plus,rotate-cw}.svg at
// https://github.com/lucide-icons/lucide/tree/1ef60af944de207e0d5176905a0759ba12e6ee8d
// Copyright (c) 2026 Lucide Icons and Contributors; Feather-derived icons:
// Copyright (c) 2013-present Cole Bemis. ISC/MIT: third_party/lucide/LICENSE.
#include "ui/theme/qtcommandicons.h"
#include <QApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>

namespace {
QPainterPath geometry(QtCommandIcon command) {
    QPainterPath p;
    const auto line = [&p](qreal x, qreal y, qreal toX, qreal toY) {
        p.moveTo(x, y);
        p.lineTo(toX, toY);
    };
    switch (command) {
    case QtCommandIcon::Add:
        line(5, 12, 19, 12);
        line(12, 5, 12, 19);
        break;
    case QtCommandIcon::Extract:
        p.moveTo(2, 7.5);
        p.lineTo(2, 5);
        p.arcTo(2, 3, 4, 4, 180, -90);
        p.lineTo(7.9, 3);
        p.cubicTo(8.58, 3, 9.21, 3.34, 9.59, 3.9);
        p.lineTo(10.4, 5.1);
        p.cubicTo(10.78, 5.66, 11.39, 6, 12.07, 6);
        p.lineTo(20, 6);
        p.arcTo(18, 6, 4, 4, 90, -90);
        p.lineTo(22, 18);
        p.arcTo(18, 16, 4, 4, 0, -90);
        p.lineTo(4, 20);
        p.cubicTo(3.07, 20, 2.26, 19.38, 2, 18.5);
        line(2, 13, 12, 13);
        p.moveTo(5, 10);
        p.lineTo(2, 13);
        p.lineTo(5, 16);
        break;
    case QtCommandIcon::Test:
        p.moveTo(20, 6);
        p.lineTo(9, 17);
        p.lineTo(4, 12);
        break;
    case QtCommandIcon::Copy:
        p.addRoundedRect(QRectF(8, 8, 14, 14), 2, 2);
        p.moveTo(4, 16);
        p.cubicTo(2.9, 16, 2, 15.1, 2, 14);
        p.lineTo(2, 4);
        p.cubicTo(2, 2.9, 2.9, 2, 4, 2);
        p.lineTo(14, 2);
        p.cubicTo(15.1, 2, 16, 2.9, 16, 4);
        break;
    case QtCommandIcon::Move:
        p.moveTo(18, 8);
        p.lineTo(22, 12);
        p.lineTo(18, 16);
        line(2, 12, 22, 12);
        break;
    case QtCommandIcon::Delete:
        line(10, 11, 10, 17);
        line(14, 11, 14, 17);
        p.moveTo(19, 6);
        p.lineTo(19, 20);
        p.arcTo(15, 18, 4, 4, 0, -90);
        p.lineTo(7, 22);
        p.arcTo(5, 18, 4, 4, 270, -90);
        p.lineTo(5, 6);
        line(3, 6, 21, 6);
        p.moveTo(8, 6);
        p.lineTo(8, 4);
        p.arcTo(8, 2, 4, 4, 180, -90);
        p.lineTo(14, 2);
        p.arcTo(12, 2, 4, 4, 90, -90);
        p.lineTo(16, 6);
        break;
    case QtCommandIcon::Info:
        p.addEllipse(QPointF(12, 12), 10, 10);
        line(12, 16, 12, 12);
        line(12, 8, 12.01, 8);
        break;
    case QtCommandIcon::Parent:
        p.moveTo(5, 12);
        p.lineTo(12, 5);
        p.lineTo(19, 12);
        line(12, 19, 12, 5);
        break;
    case QtCommandIcon::Refresh:
        p.moveTo(21, 12);
        p.arcTo(3, 3, 18, 18, 0, -270);
        p.cubicTo(14.52, 3, 16.93, 4, 18.74, 5.74);
        p.lineTo(21, 8);
        p.moveTo(21, 3);
        p.lineTo(21, 8);
        p.lineTo(16, 8);
        break;
    case QtCommandIcon::NewFolder:
        line(12, 10, 12, 16);
        line(9, 13, 15, 13);
        p.moveTo(20, 20);
        p.arcTo(18, 16, 4, 4, 270, 90);
        p.lineTo(22, 8);
        p.arcTo(18, 6, 4, 4, 0, 90);
        p.lineTo(12.1, 6);
        p.cubicTo(11.42, 6, 10.79, 5.66, 10.41, 5.1);
        p.lineTo(9.6, 3.9);
        p.cubicTo(9.22, 3.34, 8.61, 3, 7.93, 3);
        p.lineTo(4, 3);
        p.arcTo(2, 3, 4, 4, 90, 90);
        p.lineTo(2, 18);
        p.arcTo(2, 16, 4, 4, 180, 90);
        p.closeSubpath();
        break;
    }
    return p;
}

class CommandIconEngine final : public QIconEngine {
    QtCommandIcon command_;

  public:
    explicit CommandIconEngine(QtCommandIcon command) : command_(command) {}
    QIconEngine *clone() const override { return new CommandIconEngine(command_); }
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State) override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const qreal extent = qMin(rect.width(), rect.height());
        painter->translate(QRectF(rect).center() - QPointF(extent / 2, extent / 2));
        painter->scale(extent / 24, extent / 24);
        const auto group = mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
        const auto role =
            mode == QIcon::Selected ? QPalette::HighlightedText : QPalette::ButtonText;
        painter->setPen(QPen(QApplication::palette().color(group, role), 2, Qt::SolidLine,
                             Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(geometry(command_));
        painter->restore();
    }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap image(size);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        paint(&painter, QRect(QPoint(), size), mode, state);
        return image;
    }
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state,
                         qreal scale) override {
        auto result = pixmap(size * scale, mode, state);
        result.setDevicePixelRatio(scale);
        return result;
    }
};
} // namespace

QIcon QtCommandSymbol(QtCommandIcon command) { return QIcon(new CommandIconEngine(command)); }
