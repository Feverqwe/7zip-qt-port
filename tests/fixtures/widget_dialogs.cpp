#include "widget_dialogs.h"
#include <QApplication>
#include <QEvent>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include <QWindow>

WidgetDialogFixture::WidgetDialogFixture()
    : previous_(QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)) {
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    qApp->installEventFilter(this);
}

WidgetDialogFixture::~WidgetDialogFixture() {
    qApp->removeEventFilter(this);
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, previous_);
}

bool WidgetDialogFixture::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Show)
        if (auto *dialog = qobject_cast<QDialog *>(watched)) {
            if (!showLevels_.contains(dialog))
                connect(dialog, &QObject::destroyed, this,
                        [this, dialog] { showLevels_.remove(dialog); });
            showLevels_.insert(dialog, QThread::currentThread()->loopLevel());
        }
    return QObject::eventFilter(watched, event);
}

bool WidgetDialogFixture::ready(QDialog &dialog) const {
    const auto level = showLevels_.constFind(&dialog);
    if (level == showLevels_.cend() || !dialog.isVisible() ||
        QThread::currentThread()->loopLevel() <= *level || !dialog.windowHandle() ||
        !dialog.windowHandle()->isExposed())
        return false;
    if (!dialog.isActiveWindow()) {
        dialog.activateWindow();
        return false;
    }
    return true;
}

QDialog *WidgetDialogFixture::activeDialog() const {
    auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
    return dialog && ready(*dialog) ? dialog : nullptr;
}

void WidgetDialogFixture::respond(QObject &context, std::function<void(QDialog &)> callback) {
    auto *timer = new QTimer(&context);
    connectResponder(*timer, &context, [this, timer, callback = std::move(callback)] {
        if (auto *dialog = activeDialog()) {
            timer->stop();
            const QPointer<QTimer> liveTimer(timer);
            callback(*dialog);
            if (liveTimer)
                liveTimer->deleteLater();
        }
    });
    timer->start(5);
}

void WidgetDialogFixture::connectResponder(QTimer &timer, QObject *context,
                                           std::function<void()> callback) const {
    const QPointer<QTimer> liveTimer(&timer);
    connect(
        &timer, &QTimer::timeout, context,
        [liveTimer, callback = std::move(callback)] {
            if (liveTimer && liveTimer->isActive())
                callback();
        },
        Qt::QueuedConnection);
}
