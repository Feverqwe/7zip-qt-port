#include "qtstatuslabel.h"
#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

QtStatusLabel::QtStatusLabel(QWidget *parent) : QLabel(parent) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

QSize QtStatusLabel::minimumSizeHint() const {
    return QSize(0, QLabel::minimumSizeHint().height());
}

void QtStatusLabel::paintEvent(QPaintEvent *event) {
    if (sizeHint().width() <= width()) {
        QLabel::paintEvent(event);
        return;
    }
    QPainter painter(this);
    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(contentsRect(), alignment(),
                     fontMetrics().elidedText(text(), Qt::ElideRight, contentsRect().width()));
}

bool QtStatusLabel::event(QEvent *event) {
    if (event->type() == QEvent::ToolTip) {
        auto *help = static_cast<QHelpEvent *>(event);
        QToolTip::showText(help->globalPos(), text(), this);
        return true;
    }
    return QLabel::event(event);
}
