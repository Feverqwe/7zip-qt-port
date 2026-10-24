#pragma once
#include <QString>

// Read-only query; never starts an application or changes system associations.
bool QtFileHasOpenAssociation(const QString &path);
