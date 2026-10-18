#pragma once
#include <QKeySequence>
#include <QList>

class QKeyEvent;

// Decode native command keys before the source list notification.
int QtPanelVirtualKey(const QKeyEvent &key);
bool QtPanelShortcutMatches(const QKeyEvent &key, const QList<QKeySequence> &shortcuts);
