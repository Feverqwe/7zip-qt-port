#pragma once

#include "Common/MyString.h"
#include "common/collections/MyVector.h"
#include <QStringList>
#include <QThread>
#include <atomic>

struct VolumeParts {
    QStringList paths;
    qint64 totalSize = 0;
};
class QtVolumeWorker;
class CThreadSplit;
class CThreadCombine;
class CProgressSync;

class VolumeOperation : public QThread {
    Q_OBJECT

  public:
    enum class Mode { Split, Combine };
    VolumeOperation(Mode mode, QString source, QString destination, qint64 volumeSize = 0,
                    QObject *parent = nullptr);
    VolumeOperation(Mode mode, QString source, QString destination, QList<qint64> volumeSizes,
                    QObject *parent = nullptr);
    VolumeOperation(const CThreadSplit &plan, QObject *parent = nullptr);
    VolumeOperation(const CThreadCombine &plan, QObject *parent = nullptr);
    void cancel();
    bool isCanceled() const { return canceled_.load(); }
    void attachProgress(CProgressSync &sync) { progressSync_ = &sync; }
    static QList<qint64> parseSizes(const QString &text);
    static qint64 parseSize(const QString &text);
    static VolumeParts inspectParts(const QString &firstPart, QString *error);
    static QStringList discoverParts(const QString &firstPart, QString *error);

  signals:
    void advanced(qint64 bytes);
    void result(bool success, const QString &message);

  protected:
    void run() override;

  private:
    bool process(QtVolumeWorker &worker, QString *error);
    bool split(QString *error);
    bool combine(QString *error);
    Mode mode_;
    QString source_;
    QString destination_;
    CRecordVector<UInt64> volumeSizes_;
    FString volBasePath_;
    FString inputDirPrefix_;
    FStringVector partNames_;
    UInt64 numVolumes_ = 0;
    UInt64 totalSize_ = 0;
    bool nativePlan_ = false;
    bool invalidSizes_ = false;
    CProgressSync *progressSync_ = nullptr;
    std::atomic_bool canceled_{false};
};
