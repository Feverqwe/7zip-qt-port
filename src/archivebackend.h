#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>

struct ArchiveEntry {
    QString path;
    qint64 size = 0;
    qint64 packedSize = 0;
    QString modified;
    bool directory = false;
    bool link = false;
};

class ArchiveBackend : public QObject {
    Q_OBJECT

  public:
    explicit ArchiveBackend(QObject *parent = nullptr, QString executable = QString());
    virtual bool busy() const;
    virtual void list(const QString &archive);
    virtual void extract(const QString &archive, const QString &destination,
                         const QStringList &selected = {});
    virtual void create(const QString &archive, const QStringList &sources);
    virtual void cancel();

  signals:
    void entriesReady(const QList<ArchiveEntry> &entries);
    void progress(int percent, const QString &detail);
    void outputReceived(const QString &text);
    void finished(bool success, const QString &message);

  private:
    enum class Job { None, List, Extract, Create };
    void start(Job job, const QStringList &arguments, const QString &workingDirectory = {});
    void complete(int exitCode, QProcess::ExitStatus status);
    void consumeOutput(const QByteArray &chunk, bool flush = false);
    void consumeLine(const QByteArray &line);
    static QList<ArchiveEntry> parseListing(const QString &output);
    static bool safePath(const QString &path);
    bool validateExtraction(const QString &destination, QString *error) const;

    QProcess process_;
    Job job_ = Job::None;
    QString output_;
    QByteArray pendingOutput_;
    QString archive_;
    QList<ArchiveEntry> entries_;
    bool listed_ = false;
    bool cancelled_ = false;
};
