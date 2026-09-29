#include "qtlauncherprogress.h"
#include "qttaskbarprogress.h"
#include <QGuiApplication>
#include <QThread>
#include <cerrno>

class CQtTaskbarProgress::CImpl {
  public:
    std::unique_ptr<CQtLauncherProgress> Entry;

    HRESULT Check() const {
        auto *application = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        if (!application)
            return HRESULT_FROM_WIN32(ENOTSUP);
        if (QThread::currentThread() != application->thread())
            return HRESULT_FROM_WIN32(EPERM);
        return S_OK;
    }
};

CQtTaskbarProgress::CQtTaskbarProgress() : _impl(new CImpl) {}
CQtTaskbarProgress::~CQtTaskbarProgress() = default;

HRESULT CQtTaskbarProgress::HrInit() {
    const HRESULT result = _impl->Check();
    if (result != S_OK)
        return result;
    if (!_impl->Entry) {
        const QString desktopName = QGuiApplication::desktopFileName();
        if (desktopName.isEmpty())
            return E_INVALIDARG;
        _impl->Entry.reset(new CQtLauncherProgress(QDBusConnection::sessionBus(),
                                                   desktopName + QStringLiteral(".desktop")));
    }
    const HRESULT initialized = _impl->Entry->HrInit();
    if (initialized != S_OK)
        _impl->Entry.reset();
    return initialized;
}

HRESULT CQtTaskbarProgress::SetProgressState(QWidget *window, TBPFLAG state) {
    if (!window)
        return E_INVALIDARG;
    const HRESULT result = _impl->Check();
    if (result != S_OK)
        return result;
    return _impl->Entry ? _impl->Entry->SetProgressState(state) : E_FAIL;
}
HRESULT CQtTaskbarProgress::SetProgressValue(QWidget *window, UInt64 completed, UInt64 total) {
    if (!window)
        return E_INVALIDARG;
    const HRESULT result = _impl->Check();
    if (result != S_OK)
        return result;
    return _impl->Entry ? _impl->Entry->SetProgressValue(completed, total) : E_FAIL;
}
