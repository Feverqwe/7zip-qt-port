// Owned external-process controls shared by source opening and update workers.
#pragma once
#include "qtprogresstransport.h"
#include <QObject>

class ArchiveBackend;
class ArchiveProgressTransport final : public QObject, public IQtProgressTransport {
    CProgressDialog &_dialog;
    ArchiveBackend &_backend;
    bool _completed = false;

  public:
    ArchiveProgressTransport(CProgressDialog &dialog, ArchiveBackend &backend);
    ~ArchiveProgressTransport() override;
    HRESULT SetPaused(bool paused) override;
    HRESULT SetBackground(bool background) override;
};
