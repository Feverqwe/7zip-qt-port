#pragma once

#include <QAbstractItemModel>
#include <QAction>
#include <QEventLoop>
#include <QTest>
#include <QTimer>

namespace SignalWaitSupport {
// Drive queued publication continuously and stop at the public row-count signal,
// retaining the ordinary five-second QtTest wait budget.
inline bool waitForCount(QAbstractItemModel &model, int expected) {
    QTest::qWait(0);
    if (model.rowCount() == expected)
        return true;
    QEventLoop events;
    QTimer deadline;
    deadline.setSingleShot(true);
    deadline.setTimerType(Qt::PreciseTimer);
    QObject::connect(&deadline, &QTimer::timeout, &events, &QEventLoop::quit);
    const auto check = [&] {
        if (model.rowCount() == expected)
            events.quit();
    };
    QObject::connect(&model, &QAbstractItemModel::rowsInserted, &events, check);
    QObject::connect(&model, &QAbstractItemModel::rowsRemoved, &events, check);
    QObject::connect(&model, &QAbstractItemModel::modelReset, &events, check);
    deadline.start(5000);
    events.exec();
    return model.rowCount() == expected;
}
inline bool waitForEnabled(QAction &action) {
    QTest::qWait(0);
    if (action.isEnabled())
        return true;
    QEventLoop events;
    QTimer deadline;
    deadline.setSingleShot(true);
    deadline.setTimerType(Qt::PreciseTimer);
    QObject::connect(&deadline, &QTimer::timeout, &events, &QEventLoop::quit);
    QObject::connect(&action, &QAction::changed, &events, [&] {
        if (action.isEnabled())
            events.quit();
    });
    deadline.start(5000);
    events.exec();
    return action.isEnabled();
}
} // namespace SignalWaitSupport
