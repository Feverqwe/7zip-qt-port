#pragma once

class QKeyEvent;

// Decode native numeric command keys before the source list notification.
int QtPanelVirtualKey(const QKeyEvent &key);
