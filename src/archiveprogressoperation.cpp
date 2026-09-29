#include "archiveprogressoperation.h"
#include "archivebackend.h"
#include "processpriority.h"
#include <QMetaObject>
#include <QMutexLocker>
#include <QThread>

ArchiveProgressOperation::ArchiveProgressOperation(ArchiveBackend &backend,
                                                   std::function<void()> start)
    : _backend(backend), _start(std::move(start)), _callbackImpl(new CUpdateCallback100Imp) {
    _callback = _callbackImpl;
    _callbackImpl->ProgressDialog = this;
    _callbackImpl->Init();
    _callback->CompressOperation(L"");
    CompressingMode = true;
    connect(&backend, &ArchiveBackend::engineStarted, this, [this] {
        // Apply the current source policy to the newly owned child explicitly;
        // native priority inheritance differs across platforms.
        bool background;
        HRESULT result = NProcessPriority::GetBackground(0, background);
        if (result == S_OK && background)
            result = _backend.setBackground(true);
        if (result != S_OK) {
            Sync.AddError_Code_Name(result, nullptr);
            _backend.cancel();
        }
    });
    connect(&backend, &ArchiveBackend::progress, this, [this](int percent, const QString &detail) {
        if (percent >= 0 && percent <= 100)
            TransportPercent.store(percent);
        if (!detail.isEmpty()) {
            QMutexLocker lock(&_mutex);
            _detail = detail;
        }
    });
    connect(&backend, &ArchiveBackend::passwordRequired, this,
            [this] { _passwordRequested.store(true); });
    connect(&backend, &ArchiveBackend::finished, this,
            [this](bool success, const QString &message) {
                QMutexLocker lock(&_mutex);
                _success = success;
                _message = message;
                _completed = true;
            });
}

ArchiveProgressOperation::~ArchiveProgressOperation() {
    // Source Create joins the worker; queued requests cannot outlive this context.
    disconnect(&_backend, nullptr, this, nullptr);
}

HRESULT ArchiveProgressOperation::SetPausedTransport(bool paused) {
    if (_backend.busy())
        return _backend.setPaused(paused);
    QMutexLocker lock(&_mutex);
    // Before process start there is no native pause capability; after its
    // completion, restoring the prompt's prior pause no longer needs a child.
    return _completed ? S_OK : E_FAIL;
}

HRESULT ArchiveProgressOperation::SetBackgroundTransport(bool background) {
    bool previous;
    HRESULT result = NProcessPriority::GetBackground(0, previous);
    if (result != S_OK)
        return result;
    result = NProcessPriority::SetBackground(0, background);
    if (result != S_OK)
        return result;
    if (_backend.busy()) {
        result = _backend.setBackground(background);
        if (result != S_OK) {
            const HRESULT rollback = NProcessPriority::SetBackground(0, previous);
            if (rollback != S_OK)
                Sync.AddError_Code_Name(rollback, nullptr);
        }
    }
    return result;
}

HRESULT ArchiveProgressOperation::ProcessVirt() {
    WaitCreating();
    QMetaObject::invokeMethod(
        this,
        [this] {
            try {
                _start();
            } catch (...) {
                {
                    QMutexLocker lock(&_mutex);
                    _startException = std::current_exception();
                }
                // A launch callback can throw after starting the child. Quiesce
                // its transport before the native worker releases this owner.
                _backend.cancelAndWait();
                QMutexLocker lock(&_mutex);
                _completed = true;
            }
        },
        Qt::QueuedConnection);
    bool cancelled = false;
    for (;;) {
        QString detail;
        {
            QMutexLocker lock(&_mutex);
            if (_completed) {
                if (_startException)
                    std::rethrow_exception(_startException);
                if (cancelled)
                    return E_ABORT;
                if (!_success) {
                    GetMessagePair(true).Message = _message.toStdWString().c_str();
                    return E_FAIL;
                }
                return S_OK;
            }
            detail.swap(_detail);
        }
        HRESULT result = S_OK;
        if (!cancelled && _passwordRequested.exchange(false)) {
            BSTR password = nullptr;
            result = static_cast<ICryptoGetTextPassword *>(_callbackImpl)
                         ->CryptoGetTextPassword(&password);
            if (result == S_OK) {
                const QString value = QString::fromWCharArray(password);
                QMetaObject::invokeMethod(
                    this, [this, value] { _backend.providePassword(value); }, Qt::QueuedConnection);
            }
            SysFreeString(password);
        }
        if (!cancelled && result == S_OK && !detail.isEmpty()) {
            const std::wstring name = detail.toStdWString();
            // External engine completion is independent of our callback stack.
            // Publish its path with the source nonblocking setter; a blocking
            // Set_Status2/CheckStop could strand the bridge after child exit.
            Sync.Set_FilePath(name.c_str(), false);
        }
        if (!cancelled && result == S_OK && Sync.Get_Stopped())
            result = E_ABORT;
        if (!cancelled && result != S_OK) {
            cancelled = true;
            QMetaObject::invokeMethod(this, [this] { _backend.cancel(); }, Qt::QueuedConnection);
        }
        // Keep the native worker alive until QProcess completion and cleanup.
        // Native child pause is a GUI service. The completion latch is still
        // observed during pause, including finish-before-stop/prompt races.
        QThread::msleep(10);
    }
}
