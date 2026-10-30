#pragma once
#include "Common/MyString.h"
#include <QString>
class QWidget;
FString QtHelpModuleDirPrefix();
void QtDisplayHelpTopic(const FString &path, QWidget *parent);
// Explicit root allows disposable installed/unavailable help fixtures. Empty
// selects the original bundled HTML extracted from the official CHM.
void QtOpenHelpTopic(const QString &topic, QWidget *parent, const QString &root = {});
void QtOpenHelpText(const QString &title, const QString &text, QWidget *parent);
