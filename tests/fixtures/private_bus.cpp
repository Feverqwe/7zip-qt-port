#include "private_bus.h"
#include <QDebug>
#include <QUuid>

bool CPrivateBus::Start() {
    if (!_directory.isValid())
        return false;
    _daemon.start(QString::fromUtf8(DBUS_DAEMON_PATH),
                  {"--session", "--nofork", "--nopidfile", "--print-address=1",
                   "--address=unix:tmpdir=" + _directory.path()});
    if (!_daemon.waitForStarted(3000))
        return false;
    for (unsigned i = 0; !_daemon.canReadLine() && i < 3; ++i)
        if (!_daemon.waitForReadyRead(1000)) {
            qWarning().noquote() << _daemon.errorString() << _daemon.readAllStandardError();
            return false;
        }
    Address = QString::fromUtf8(_daemon.readLine().trimmed());
    return Address.startsWith("unix:");
}
QDBusConnection CPrivateBus::Connect() {
    const QString name = QUuid::createUuid().toString(QUuid::Id128);
    _names.append(name);
    return QDBusConnection::connectToBus(Address, name);
}
void CPrivateBus::Stop() {
    if (_daemon.state() != QProcess::NotRunning) {
        _daemon.terminate();
        if (!_daemon.waitForFinished(1000)) {
            _daemon.kill();
            _daemon.waitForFinished(1000);
        }
    }
}
CPrivateBus::~CPrivateBus() {
    for (const auto &name : _names)
        QDBusConnection::disconnectFromBus(name);
    Stop();
}
