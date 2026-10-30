#pragma once
#include <QMetaType>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include <memory>
#include <optional>

class ArchiveMountDecision final {
    QMutex mutex_;
    QWaitCondition ready_;
    std::optional<bool> answer_;

  public:
    void answer(bool allow);
    bool wait(const std::atomic_bool &cancelled);
};
using ArchiveMountDecisionPtr = std::shared_ptr<ArchiveMountDecision>;
Q_DECLARE_METATYPE(ArchiveMountDecisionPtr)
