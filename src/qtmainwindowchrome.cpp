#include "qtmainwindowchrome.h"
#include <QApplication>
#include <QPainter>
#include <QProxyStyle>
#include <QPushButton>
#include <QStyleOptionToolButton>
#include <QToolBar>
#include <QToolButton>

namespace {
class MainChromeStyle final : public QProxyStyle {
  public:
    explicit MainChromeStyle(QWidget &owner) { setParent(&owner); }

    int pixelMetric(PixelMetric metric, const QStyleOption *option,
                    const QWidget *widget) const override {
        if (metric == PM_ToolBarItemMargin)
            return 8;
        if (metric == PM_ToolBarItemSpacing)
            return 4;
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                           const QWidget *widget) const override {
        const QSize native = QProxyStyle::sizeFromContents(type, option, size, widget);
        if (type != CT_ToolButton)
            return native;
        const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option);
        if (!tool)
            return native;
        const int horizontalPadding = 16;
        const int labelGap = tool->toolButtonStyle == Qt::ToolButtonTextBesideIcon ? 2 : 0;
        return native.expandedTo(
            QSize(size.width() + horizontalPadding + labelGap, qMax(26, size.height() + 8)));
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget) const override {
        if (element != PE_IndicatorToolBarSeparator) {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }
        painter->save();
        QColor line = option->palette.color(QPalette::WindowText);
        line.setAlphaF(0.22);
        painter->setPen(line);
        const QPoint center = option->rect.center();
        const int halfHeight = qMin(10, option->rect.height() / 2);
        painter->drawLine(center + QPoint(0, -halfHeight), center + QPoint(0, halfHeight));
        painter->restore();
    }

    void drawControl(ControlElement control, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget) const override {
        const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option);
        if (control != CE_ToolButtonLabel || !tool) {
            QProxyStyle::drawControl(control, option, painter, widget);
            return;
        }
        // Cocoa's toolbar label uses a small bold font even when the widget
        // carries the application font. Keep native panel/focus painting, but
        // draw this label with the same regular font as the surrounding UI.
        painter->save();
        painter->setFont(widget ? widget->font() : QApplication::font());
        const bool text = tool->toolButtonStyle == Qt::ToolButtonTextBesideIcon;
        const int textWidth = text ? painter->fontMetrics().horizontalAdvance(tool->text) : 0;
        const int width = tool->iconSize.width() + (text ? 6 + textWidth : 0);
        const QRect row(tool->rect.center().x() - width / 2, tool->rect.top(), width,
                        tool->rect.height());
        QRect icon(row.left(), row.center().y() - tool->iconSize.height() / 2,
                   tool->iconSize.width(), tool->iconSize.height());
        icon = visualRect(tool->direction, tool->rect, icon);
        const bool enabled = tool->state & State_Enabled;
        tool->icon.paint(painter, icon, Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        if (text) {
            const QRect label = visualRect(
                tool->direction, tool->rect,
                QRect(row.left() + tool->iconSize.width() + 6, row.top(), textWidth, row.height()));
            drawItemText(painter, label, Qt::AlignVCenter | Qt::AlignLeft, tool->palette, enabled,
                         tool->text, QPalette::ButtonText);
        }
        painter->restore();
    }

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override {
        const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option);
        if (control != CC_ToolButton || !tool) {
            QProxyStyle::drawComplexControl(control, option, painter, widget);
            return;
        }
        QStyleOptionToolButton panel(*tool);
        panel.icon = {};
        panel.text.clear();
        QProxyStyle::drawComplexControl(control, &panel, painter, widget);
        drawControl(CE_ToolButtonLabel, tool, painter, widget);
    }
};
} // namespace

void QtConfigureMainToolbar(QToolBar &toolbar) {
    toolbar.setStyle(new MainChromeStyle(toolbar));
    toolbar.setFont(QApplication::font());
}

void QtConfigureMainToolbarButton(QToolButton &button) {
    button.setStyle(button.parentWidget()->style());
    button.setFont(QApplication::font());
}

void QtConfigurePanelParentButton(QPushButton &button) {
    button.setIconSize(QSize(16, 16));
    button.setMinimumSize(42, 26);
    button.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button.setFont(QApplication::font());
#ifdef Q_OS_MACOS
    button.setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
}
