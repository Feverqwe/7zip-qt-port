#pragma once
#include <QDBusConnection>
#include <QProcess>
#include <QTemporaryDir>

// Owns a bounded local bus fixture; never connects to the user's session bus.
class CPrivateBus {
    QProcess _daemon;
    QStringList _names;
    QTemporaryDir _directory;

  public:
    QString Address;
    bool Start();
    QDBusConnection Connect();
    void Stop();
    ~CPrivateBus();
};
