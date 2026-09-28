#pragma once

#include <QStringView>

bool DoesWildcardMatchName(const char16_t *mask, const char16_t *name);
bool DoesWildcardMatchName(QStringView mask, QStringView name);

void SplitPathToParts_Smart(const QString &path, QString &dirPrefix, QString &name);
