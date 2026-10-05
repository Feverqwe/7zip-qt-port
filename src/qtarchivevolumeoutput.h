#pragma once
#include "Common/MyTypes.h"
#include "Common/MyVector.h"
#include "Common/MyWindows.h"
#include "qtworkerprogress.h"
#include <QStringList>
#include <QThread>
#include <functional>

// CLI integration: split the completed engine output using the original
// CMultiOutStream, then publish its complete set without replacing any target.
class QtArchiveVolumeOutput final : public QThread {
  public:
    QtArchiveVolumeOutput(QString source, QString target, CRecordVector<UInt64> sizes,
                          bool setArcMTime = false, QString sfxModule = {}, QString sfxTarget = {});
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
    QString sfxModule_, sfxTarget_;
    CRecordVector<UInt64> sizes_;
    QtWorkerProgress progress_;
    bool setArcMTime_ = false;
};
