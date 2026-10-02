#pragma once

#include "archivecodeccatalog.h"
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <functional>
#include <memory>

struct ArchiveCodecProbeResult {
    HRESULT result = E_FAIL;
    QString error;
    std::shared_ptr<const ArchiveCodecCatalog> catalog;
};

// Process-version/discovery glue; format semantics remain in native CCodecs.
class ArchiveCodecProbe final : public QObject {
    QProcess process_;
    QTimer deadline_;
    QByteArray output_;
    QString module_;
    std::function<void(ArchiveCodecProbeResult)> completion_;
    bool active_ = false;
    bool cancelled_ = false;
    bool timedOut_ = false;
    bool explicitModule_ = false;
    void Finish(int exitCode, QProcess::ExitStatus status);

  public:
    explicit ArchiveCodecProbe(QObject *parent = nullptr);
    ~ArchiveCodecProbe() override;
    bool Start(const QString &engine, const QString &module,
               std::function<void(ArchiveCodecProbeResult)> completion, int timeoutMs = 5000);
    void Cancel();
    bool active() const { return active_; }
};
