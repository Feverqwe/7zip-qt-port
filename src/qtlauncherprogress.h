// Native launcher transport for the source taskbar presentation adapter.
#pragma once
#include "Common/MyWindows.h"
#include "qttaskbarprogress.h"
#include <QDBusConnection>
#include <QObject>
#include <QVariantMap>

class QDBusServiceWatcher;

// Owns one launcher entry on an explicit bus, never selects a bus implicitly.
// A successful transport call does not establish a compatible visible launcher.
class CQtLauncherProgress : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.Unity.LauncherEntry")
    QDBusConnection _connection;
    QString _desktopFile;
    QString _objectPath;
    QVariantMap _properties;
    QDBusServiceWatcher *_watcher = nullptr;
    TBPFLAG _state = TBPF_NOPROGRESS;
    bool _registered = false;
    HRESULT Check() const;
    HRESULT Publish(const QVariantMap &properties);
    HRESULT Apply(const QVariantMap &properties);

  public:
    CQtLauncherProgress(const QDBusConnection &connection, const QString &desktopFile);
    ~CQtLauncherProgress() override;
    HRESULT HrInit();
    HRESULT SetProgressState(TBPFLAG state);
    HRESULT SetProgressValue(UInt64 completed, UInt64 total);

  public slots:
    // Native protocol method, required on the same object path as Update.
    Q_SCRIPTABLE void Query(QString &appUri, QVariantMap &properties);

  signals:
    // Published through QDBusConnection::send so transport failure is observable.
    Q_SCRIPTABLE void Update(const QString &appUri, const QVariantMap &properties);
};
