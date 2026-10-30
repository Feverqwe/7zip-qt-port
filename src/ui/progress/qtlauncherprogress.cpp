#include "ui/progress/qtlauncherprogress.h"
#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QThread>
#include <QUuid>
#include <algorithm>
#include <cerrno>

namespace {
const QString kInterface = QStringLiteral("com.canonical.Unity.LauncherEntry");
}

CQtLauncherProgress::CQtLauncherProgress(const QDBusConnection &connection,
                                         const QString &desktopFile)
    : _connection(connection), _desktopFile(desktopFile),
      _objectPath(QStringLiteral("/com/canonical/unity/launcherentry/p") +
                  QUuid::createUuid().toString(QUuid::Id128)) {
    _properties.insert(QStringLiteral("progress"), 0.0);
    _properties.insert(QStringLiteral("progress-visible"), false);
    _properties.insert(QStringLiteral("urgent"), false);
}

CQtLauncherProgress::~CQtLauncherProgress() {
    if (_registered) {
        SetProgressState(TBPF_NOPROGRESS);
        _connection.unregisterObject(_objectPath);
    }
}

HRESULT CQtLauncherProgress::Check() const {
    if (QThread::currentThread() != thread())
        return HRESULT_FROM_WIN32(EPERM);
    if (!_connection.isConnected())
        return HRESULT_FROM_WIN32(ENOTCONN);
    return _registered ? S_OK : E_FAIL;
}

HRESULT CQtLauncherProgress::Publish(const QVariantMap &properties) {
    const HRESULT result = Check();
    if (result != S_OK)
        return result;
    QDBusMessage message = QDBusMessage::createSignal(_objectPath, kInterface, "Update");
    message << (QStringLiteral("application://") + _desktopFile) << properties;
    return _connection.send(message) ? S_OK : E_FAIL;
}

HRESULT CQtLauncherProgress::Apply(const QVariantMap &properties) {
    const HRESULT result = Check();
    if (result != S_OK)
        return result;
    QVariantMap changed;
    for (auto entry = properties.cbegin(); entry != properties.cend(); ++entry)
        if (_properties.value(entry.key()) != entry.value())
            changed.insert(entry.key(), entry.value());
    if (changed.isEmpty())
        return S_OK;
    const HRESULT published = Publish(changed);
    if (published == S_OK)
        for (auto entry = changed.cbegin(); entry != changed.cend(); ++entry)
            _properties.insert(entry.key(), entry.value());
    return published;
}

HRESULT CQtLauncherProgress::HrInit() {
    if (QThread::currentThread() != thread())
        return HRESULT_FROM_WIN32(EPERM);
    if (_desktopFile.isEmpty() || !_desktopFile.endsWith(QLatin1String(".desktop")) ||
        _desktopFile.contains(QLatin1Char('/')) || _desktopFile.contains(QLatin1Char('\\')) ||
        _desktopFile.contains(QChar(0)))
        return E_INVALIDARG;
    if (!_connection.isConnected())
        return HRESULT_FROM_WIN32(ENOTCONN);
    if (_registered)
        return S_OK;
    if (!_connection.registerObject(_objectPath, this,
                                    QDBusConnection::ExportScriptableSlots |
                                        QDBusConnection::ExportScriptableSignals))
        return E_FAIL;
    _registered = true;
    _watcher = new QDBusServiceWatcher(QStringLiteral("com.canonical.Unity"), _connection,
                                       QDBusServiceWatcher::WatchForRegistration, this);
    connect(_watcher, &QDBusServiceWatcher::serviceRegistered, this,
            [this] { Publish(_properties); });
    const HRESULT result = Publish(_properties);
    if (result != S_OK) {
        delete _watcher;
        _watcher = nullptr;
        _connection.unregisterObject(_objectPath);
        _registered = false;
    }
    return result;
}

HRESULT CQtLauncherProgress::SetProgressState(TBPFLAG state) {
    // This native protocol has no indeterminate or colored-bar property.
    if (state == TBPF_INDETERMINATE)
        return HRESULT_FROM_WIN32(ENOTSUP);
    if (state != TBPF_NOPROGRESS && state != TBPF_NORMAL && state != TBPF_ERROR &&
        state != TBPF_PAUSED)
        return E_INVALIDARG;
    const HRESULT result = Check();
    if (result != S_OK)
        return result;
    QVariantMap changed;
    changed.insert(QStringLiteral("progress-visible"), state != TBPF_NOPROGRESS);
    changed.insert(QStringLiteral("urgent"), state == TBPF_ERROR);
    const HRESULT published = Apply(changed);
    if (published == S_OK)
        _state = state;
    return published;
}

HRESULT CQtLauncherProgress::SetProgressValue(UInt64 completed, UInt64 total) {
    if (total == 0)
        return E_INVALIDARG;
    const HRESULT result = Check();
    if (result != S_OK)
        return result;
    QVariantMap changed;
    changed.insert(QStringLiteral("progress"), double(std::min(completed, total)) / double(total));
    changed.insert(QStringLiteral("progress-visible"), true);
    changed.insert(QStringLiteral("urgent"), _state == TBPF_ERROR);
    const HRESULT published = Apply(changed);
    if (published == S_OK) {
        if (_state != TBPF_PAUSED && _state != TBPF_ERROR)
            _state = TBPF_NORMAL;
    }
    return published;
}

void CQtLauncherProgress::Query(QString &appUri, QVariantMap &properties) {
    appUri = QStringLiteral("application://") + _desktopFile;
    properties = _properties;
}
