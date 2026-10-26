#include "qtfilelisthover.h"

#include <QApplication>
#include <QMouseEvent>
#include <QStyle>
#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

QtFileListHover::QtFileListHover(QAbstractItemView &view) : QObject(&view) {
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, [this] {
        auto *view = qobject_cast<QAbstractItemView *>(parent());
        if (!enabled_ || suspended_ || !view || !view->isEnabled() || !view->isVisible() ||
            !index_.isValid() || view->indexAt(position_).siblingAtColumn(0) != index_ ||
            QApplication::mouseButtons() != Qt::NoButton ||
            QApplication::keyboardModifiers() != Qt::NoModifier)
            return;
        view->selectionModel()->setCurrentIndex(index_, QItemSelectionModel::ClearAndSelect |
                                                            QItemSelectionModel::Rows);
    });
    view.viewport()->installEventFilter(this);
}

void QtFileListHover::SetEnabled(bool enabled) {
    enabled_ = enabled;
    if (auto *view = qobject_cast<QAbstractItemView *>(parent())) {
        view->setMouseTracking(enabled);
        view->viewport()->setMouseTracking(enabled);
    }
    Cancel();
}

void QtFileListHover::Cancel() {
    timer_.stop();
    index_ = QPersistentModelIndex();
}

bool QtFileListHover::eventFilter(QObject *watched, QEvent *event) {
    // QWidget can deliver viewport events after the QAbstractItemView part of
    // its destructor has finished. QObject parenting survives that phase;
    // qobject_cast checks the live type before accessing any view service.
    auto *view = qobject_cast<QAbstractItemView *>(parent());
    if (!view || watched != view->viewport())
        return false;
    if (event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::Wheel || event->type() == QEvent::Hide ||
        event->type() == QEvent::EnabledChange) {
        Cancel();
        return false;
    }
    if (event->type() != QEvent::MouseMove || !enabled_ || suspended_ || !view->isEnabled())
        return false;
    const auto *mouse = static_cast<QMouseEvent *>(event);
    position_ = mouse->position().toPoint();
    const QModelIndex current = view->indexAt(position_).siblingAtColumn(0);
    if (mouse->buttons() != Qt::NoButton || mouse->modifiers() != Qt::NoModifier ||
        !current.isValid()) {
        Cancel();
    } else if (current != index_) {
        index_ = current;
        int delay = view->style()->styleHint(QStyle::SH_ToolTip_WakeUpDelay, nullptr, view);
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
