#pragma once
#include <QString>

// Objective-C++ boundary contains no upstream MyWindows BOOL typedef.
struct QtMacAssociationState {
    QString type;
    QString bundle;
    QString path;
};
QString QtMacOwnBundlePath();
QString QtMacOwnBundleIdentifier();
QString QtMacCurrentUserName();
bool QtMacCanSetAssociation();
long QtMacReadAssociation(const QString &extension, QtMacAssociationState &state);
long QtMacSetAssociation(const QString &extension, const QString &applicationPath);
