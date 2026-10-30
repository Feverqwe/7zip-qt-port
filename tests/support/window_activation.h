#pragma once

#include <QApplication>
#include <QDeadlineTimer>
#include <QTimer>
#include <QWidget>
#include <QtTest>

namespace WindowActivation {
inline bool waitForActive(QWidget &window) {
    // Keep exposure and repeated activation requests inside the original
    // five-second wait while native window events are delivered.
    const QDeadlineTimer deadline(5000, Qt::PreciseTimer);
    if (!QTest::qWaitForWindowExposed(&window, deadline))
        return false;
    QTimer retry;
    QObject::connect(&retry, &QTimer::timeout, &window, [&window] {
        if (!window.isActiveWindow()) {
            window.raise();
            window.activateWindow();
        }
    });
    retry.start(10);
    window.raise();
    window.activateWindow();
    const bool active = QTest::qWaitForWindowActive(&window, deadline);
    if (!active)
        qWarning() << "Window activation failed:" << "application state"
                   << QGuiApplication::applicationState() << "visible" << window.isVisible()
                   << "active window" << QApplication::activeWindow() << "modal"
                   << QApplication::activeModalWidget() << "popup"
                   << QApplication::activePopupWidget();
    return active;
}
} // namespace WindowActivation
