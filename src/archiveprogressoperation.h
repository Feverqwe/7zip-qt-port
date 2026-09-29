// External-engine transport boundary for the upstream progress worker/callback.
#pragma once
#include "CPP/7zip/UI/FileManager/UpdateCallback100.h"
#include <QMutex>
#include <QString>
#include <atomic>
#include <exception>
#include <functional>

class ArchiveBackend;
class ArchiveProgressTransport;
class ArchiveProgressOperation : public CProgressThreadVirt {
    ArchiveBackend &_backend;
    std::function<void()> _start;
    CMyComPtr<IFolderArchiveUpdateCallback> _callback;
    CUpdateCallback100Imp *_callbackImpl;
    std::unique_ptr<ArchiveProgressTransport> _transport;
    QMutex _mutex;
    QString _message;
    bool _completed = false;
    bool _success = false;
    std::exception_ptr _startException;
    std::atomic<bool> _passwordRequested{false};
    HRESULT ProcessVirt() override;

  public:
    ArchiveProgressOperation(ArchiveBackend &backend, std::function<void()> start);
    ~ArchiveProgressOperation() override;
    bool succeeded() const { return _success; } // Read after source Create joins.
    QString resultMessage() const { return _message; }
};
