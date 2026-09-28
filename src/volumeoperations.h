#pragma once

#include <QStringList>
#include <QThread>
#include <atomic>

class QFile;

struct VolumeParts {
    QStringList paths;
    qint64 totalSize = 0;
};

class VolumeOperation : public QThread {
    Q_OBJECT

  public:
    enum class Mode { Split, Combine };
    VolumeOperation(Mode mode, QString source, QString destination, qint64 volumeSize = 0,
                    QObject *parent = nullptr);
    VolumeOperation(Mode mode, QString source, QString destination, QList<qint64> volumeSizes,
                    QObject *parent = nullptr);
    void cancel();
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
    bool split(QString *error);
    bool combine(QString *error);
    bool copyBytes(QFile &input, QFile &output, qint64 count, qint64 *progress, QString *error);
    Mode mode_;
    QString source_;
    QString destination_;
    QList<qint64> volumeSizes_;
    std::atomic_bool canceled_{false};
};
