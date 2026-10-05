#pragma once
#include "Common/MyWindows.h"
#include "qtworkerprogress.h"
#include <QString>
#include <QThread>
#include <atomic>
#include <memory>

class QtOwnedTempDir;

// Scheduling/ownership adapter for source CCopyStateIO::MyCopyFile.
class QtArchiveCopyPreparation final : public QThread {
    QString source_;
    QString output_;
    std::unique_ptr<QtOwnedTempDir> staging_;
    QtWorkerProgress progress_;
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
