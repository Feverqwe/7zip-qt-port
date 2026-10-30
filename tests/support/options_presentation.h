#pragma once
#include "ui/controls/qtdisclosure.h"
#include <QApplication>
#include <QScrollArea>
#include <QTabWidget>

namespace OptionsPresentation {
inline bool show(QWidget &dialog, QWidget *control) {
    if (!control)
        return false;
    QList<QScrollArea *> scrolls;
    for (auto *parent = control->parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto *group = qobject_cast<QtDisclosure *>(parent))
            group->setExpanded(true);
        if (auto *scroll = qobject_cast<QScrollArea *>(parent)) {
            auto *tabs = dialog.findChild<QTabWidget *>("optionsPages");
            if (tabs->indexOf(scroll) >= 0)
                tabs->setCurrentWidget(scroll);
            scrolls.append(scroll);
        }
    }
    QApplication::processEvents();
    const auto ensure = [control](QScrollArea *scroll) {
        const QPoint center = control->mapTo(scroll->widget(), control->rect().center());
        scroll->ensureVisible(center.x(), center.y(), control->width() / 2 + 1,
                              control->height() / 2 + 1);
    };
    for (auto *scroll : scrolls)
        ensure(scroll);
    QApplication::processEvents();
    for (auto *scroll : scrolls)
        ensure(scroll);
    QApplication::processEvents();
    return control->isVisibleTo(&dialog);
}
inline bool show(QWidget &dialog, const char *name) {
    return show(dialog, dialog.findChild<QWidget *>(name));
}
} // namespace OptionsPresentation
