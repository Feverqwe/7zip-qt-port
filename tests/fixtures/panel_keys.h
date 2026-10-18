#pragma once
#include <QAction>
#include <QToolBar>
#include <QtTest>

namespace PanelKeys {
inline void command(QWidget *view, Qt::Key key, Qt::KeyboardModifiers modifiers) {
    QTest::keyClick(view, key, modifiers);
    // A synthetic keyClick supplies modifiers on both key events. Release the
    // modifier keys too, as a physical chord does, before the next interaction.
    if (modifiers.testFlag(Qt::ShiftModifier))
        QTest::keyRelease(view, Qt::Key_Shift);
    if (modifiers.testFlag(Qt::ControlModifier))
        QTest::keyRelease(view, Qt::Key_Control);
    if (modifiers.testFlag(Qt::AltModifier))
        QTest::keyRelease(view, Qt::Key_Alt);
}
inline void openSelection(QWidget *view) {
#ifdef Q_OS_MACOS
    command(view, Qt::Key_Down, Qt::ControlModifier);
#else
    QTest::keyClick(view, Qt::Key_Return);
#endif
}

inline void deletePermanently(QWidget *view, QAction *command) {
#ifdef Q_OS_MACOS
    // The redesign intentionally reserves Command-Shift-Backspace. The source
    // Shift/menu action still provides explicit deletion without the recycle bin.
    auto *toolbar = view->window()->findChild<QToolBar *>("archiveToolbar");
    QVERIFY(toolbar);
    QWidget *button = toolbar->widgetForAction(command);
    QVERIFY(button);
    QTest::mouseClick(button, Qt::LeftButton, Qt::ShiftModifier);
    QTest::keyRelease(view, Qt::Key_Shift);
#else
    Q_UNUSED(command);
    QTest::keyClick(view, Qt::Key_Delete, Qt::ShiftModifier);
#endif
}
} // namespace PanelKeys
