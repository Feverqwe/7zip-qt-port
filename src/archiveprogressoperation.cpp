#include "archiveprogressoperation.h"
#include "archivebackend.h"
#include "archiveprogresstransport.h"
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
    _transport = std::make_unique<ArchiveProgressTransport>(*this, backend);
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
        {
            QMutexLocker lock(&_mutex);
            if (_completed) {
                if (_startException)
                    std::rethrow_exception(_startException);
                if (cancelled || Sync.Get_Stopped())
                    return E_ABORT;
                if (!_success) {
                    GetMessagePair(true).Message = _message.toStdWString().c_str();
                    return E_FAIL;
                }
                return S_OK;
            }
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
