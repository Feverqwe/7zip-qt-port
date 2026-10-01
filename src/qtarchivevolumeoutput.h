#pragma once
#include "Common/MyVector.h"
#include "Common/MyWindows.h"
#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QWaitCondition>
#include <functional>

// CLI integration: split the completed engine output using the original
// CMultiOutStream, then publish its complete set without replacing any target.
class QtArchiveVolumeOutput final : public QThread {
  public:
    QtArchiveVolumeOutput(QString source, QString target, CRecordVector<UInt64> sizes,
                          bool setArcMTime = false);
    void cancel();
    void setPaused(bool paused);
    HRESULT checkpoint();
    std::function<void(quint64, quint64)> Progress;
    HRESULT Result = E_FAIL;
    QString Error;
    QStringList PublishedPaths;

  private:
    void run() override;
    HRESULT createVolumes();
    QString source_, target_;
    CRecordVector<UInt64> sizes_;
    QMutex mutex_;
    QWaitCondition resumed_;
    bool stopped_ = false;
    bool paused_ = false;
    bool setArcMTime_ = false;
};
