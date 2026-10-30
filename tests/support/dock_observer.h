#pragma once
#include <QString>

// AppKit observations isolated from MyWindows.h's incompatible BOOL typedef.
bool EnsureCocoaDock();
QString NativeDockBadge();
void SetNativeDockBadge(const QString &text);
