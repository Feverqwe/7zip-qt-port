#pragma once
#include "qtarchivedeleteinputs.h"
#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QWaitCondition>

// Prepare a native snapshot before creation, then run native deletion only
// after every CLI stage and output-volume publication has succeeded.
class QtArchiveCreationLifecycle final : public QThread {
  public:
    explicit QtArchiveCreationLifecycle(QStringList sources, const CBoolPair &symLinks = {});
    void beginDeletion();
    void cancel();
    void setPaused(bool paused);
    HRESULT checkpoint();
    HRESULT Result = E_FAIL;
    QString Error;
    std::function<void(const QString &, bool)> Progress;

  private:
    void run() override;
    HRESULT scan();
    QtArchiveDeleteInputs inputs_;
    QStringList sources_;
    QMutex mutex_;
    QWaitCondition resumed_;
    bool deleting_ = false;
    bool paused_ = false;
    bool stopped_ = false;
};
