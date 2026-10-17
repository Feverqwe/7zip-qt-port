#include "qtfilelisthover.h"

#include <QApplication>
#include <QMouseEvent>
#include <QStyle>
#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

QtFileListHover::QtFileListHover(QAbstractItemView &view) : QObject(&view), view_(&view) {
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, [this] {
        if (!enabled_ || suspended_ || !view_ || !view_->isVisible() || !index_.isValid() ||
            view_->indexAt(position_).siblingAtColumn(0) != index_ ||
            QApplication::mouseButtons() != Qt::NoButton ||
            QApplication::keyboardModifiers() != Qt::NoModifier)
            return;
        view_->selectionModel()->setCurrentIndex(index_, QItemSelectionModel::ClearAndSelect |
                                                             QItemSelectionModel::Rows);
    });
    view.viewport()->installEventFilter(this);
}

void QtFileListHover::SetEnabled(bool enabled) {
    enabled_ = enabled;
    if (view_) {
        view_->setMouseTracking(enabled);
        view_->viewport()->setMouseTracking(enabled);
    }
    Cancel();
}

void QtFileListHover::Cancel() {
    timer_.stop();
    index_ = QPersistentModelIndex();
}

bool QtFileListHover::eventFilter(QObject *watched, QEvent *event) {
    if (!view_ || watched != view_->viewport())
        return false;
    if (event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::Wheel || event->type() == QEvent::Hide) {
        Cancel();
        return false;
    }
    if (event->type() != QEvent::MouseMove || !enabled_ || suspended_)
        return false;
    const auto *mouse = static_cast<QMouseEvent *>(event);
    position_ = mouse->position().toPoint();
    const QModelIndex current = view_->indexAt(position_).siblingAtColumn(0);
    if (mouse->buttons() != Qt::NoButton || mouse->modifiers() != Qt::NoModifier ||
        !current.isValid()) {
        Cancel();
    } else if (current != index_) {
        index_ = current;
        int delay = view_->style()->styleHint(QStyle::SH_ToolTip_WakeUpDelay, nullptr, view_);
#ifdef Q_OS_WIN
        // The source ListView uses the system hover timeout by default.
        UINT hoverTime;
        if (SystemParametersInfoW(SPI_GETMOUSEHOVERTIME, 0, &hoverTime, 0))
            delay = int(hoverTime);
#endif
        timer_.start(qMax(0, delay));
    }
    return false;
}
