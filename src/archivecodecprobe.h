#pragma once

#include "archivecodeccatalog.h"
#include <QObject>
#include <QThread>
#include <functional>
#include <memory>

struct ArchiveCodecProbeResult {
    HRESULT result = E_FAIL;
    QString error;
    std::shared_ptr<const ArchiveCodecCatalog> catalog;
};

// Process-version/discovery glue; format semantics remain in native CCodecs.
class ArchiveCodecProbe final : public QObject {
    struct Work;
    std::unique_ptr<QThread> loader_;
    std::shared_ptr<Work> work_;
    std::function<void(ArchiveCodecProbeResult)> completion_;
    bool active_ = false;
    static ArchiveCodecProbeResult Discover(const QString &engine, const QString &module,
                                            const std::shared_ptr<Work> &work);
    void Complete(ArchiveCodecProbeResult result);

  public:
    explicit ArchiveCodecProbe(QObject *parent = nullptr);
    ~ArchiveCodecProbe() override;
    bool Start(const QString &engine, const QString &module,
               std::function<void(ArchiveCodecProbeResult)> completion, int timeoutMs = 5000);
    void Cancel();
    bool active() const { return active_; }
};
