#pragma once
#include "Common/MyWindows.h"
#include <QMutex>
#include <QString>
#include <QThread>
#include <QWaitCondition>
#include <atomic>
#include <memory>

class QtOwnedTempDir;

// Scheduling/ownership adapter for source CCopyStateIO::MyCopyFile.
class QtArchiveCopyPreparation final : public QThread {
    QString source_;
    QString output_;
    std::unique_ptr<QtOwnedTempDir> staging_;
    QMutex mutex_;
    QWaitCondition resumed_;
    bool paused_ = false;
    bool canceled_ = false;
    std::atomic<quint64> completed_{0};
    quint64 total_ = 0;
    HRESULT checkpoint();
    void run() override;

  public:
    QtArchiveCopyPreparation(QString source, QString output);
    ~QtArchiveCopyPreparation() override;
    void cancel();
    void setPaused(bool paused);
    int percent() const;
    HRESULT publish(); // Called only after joining; transfers the prepared file.
    HRESULT Result = E_FAIL;
};
