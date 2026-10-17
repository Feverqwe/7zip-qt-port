#include "archiveprogresstransport.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivebackend.h"
#include "processpriority.h"

ArchiveProgressTransport::ArchiveProgressTransport(CProgressDialog &dialog, ArchiveBackend &backend,
                                                   bool nativeWorker)
    : _dialog(dialog), _backend(backend), _nativeWorker(nativeWorker) {
    _dialog.AttachTransport(this);
    connect(&backend, &ArchiveBackend::phaseStarted, this, [this] {
        bool background;
        HRESULT result = NProcessPriority::GetBackground(0, background);
        if (result == S_OK && background)
            result = _backend.setBackground(true);
        if (result == S_OK)
            result = _backend.setPaused(_dialog.Sync.Get_Paused());
        if (result != S_OK) {
            _dialog.Sync.AddError_Code_Name(result, nullptr);
            _backend.cancel();
        }
    });
    connect(&backend, &ArchiveBackend::progress, this, [this](int percent, const QString &detail) {
        _dialog.SetTransportPercent(percent);
        if (!detail.isEmpty()) {
            const std::wstring name = detail.toStdWString();
            _dialog.Sync.Set_FilePath(name.c_str(), false);
        }
    });
    connect(&backend, &ArchiveBackend::finished, this, [this] { _completed = true; });
}

ArchiveProgressTransport::~ArchiveProgressTransport() { _dialog.AttachTransport(nullptr); }

HRESULT ArchiveProgressTransport::SetPaused(bool paused) {
    if (_backend.busy())
        return _backend.setPaused(paused);
    // Native source callbacks wait on the dialog's Sync pause state. A native
    // phase has no child process to suspend; metadata reopen still uses above.
    return _nativeWorker || _completed ? S_OK : E_FAIL;
}

void ArchiveProgressTransport::Cancel() { _backend.cancel(); }

HRESULT ArchiveProgressTransport::SetBackground(bool background) {
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
                _dialog.Sync.AddError_Code_Name(rollback, nullptr);
        }
    }
    return result;
}
