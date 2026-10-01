#pragma once

#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QWaitCondition>
#include <atomic>
#include <functional>

class FileTransferOperation : public QThread {
    Q_OBJECT

  public:
    enum class DestinationKind { Directory, DirectPath };
    enum class ResultKind { Success, Failure, Canceled };
    struct FileSystemAdapter {
        std::function<bool(const QString &, const QString &)> trySourceRename;
        std::function<bool(const QString &)> removeSource;
    };
    FileTransferOperation(QStringList sources, QString destination, bool move,
                          QObject *parent = nullptr, FileSystemAdapter adapter = {},
                          DestinationKind destinationKind = DestinationKind::Directory);
    void cancel();
    void submitOverwriteAnswer(int answer);
    bool isCanceled() const { return canceled_; }
    ResultKind resultKind() const { return resultKind_; }

  signals:
    void currentFile(const QString &path);
    void advanced(qint64 bytes);
    void result(bool success, const QString &message);
    void overwriteRequested(const QString &existing, const QString &incoming);
    void dismissOverwriteRequested();

  protected:
    void run() override;

  private:
    bool copyEntry(const QString &source, const QString &target, QString *error,
                   bool reportProgress = true);
    bool mergeEntry(const QString &source, const QString &stage, const QString &target,
                    const std::function<int(const QString &, const QString &)> &answer,
                    QStringList *writtenSources, QString *error);
    bool removeMergedSource(const QString &source, const QStringList &writtenSources,
                            QString *error);
    bool validateEntry(const QString &source, QString *error);
    static bool removeEntry(const QString &path);
    int askOverwrite(const QString &existing, const QString &incoming);

    QStringList sources_;
    QString destination_;
    bool move_;
    DestinationKind destinationKind_;
    FileSystemAdapter adapter_;
    std::atomic_bool canceled_{false};
    std::atomic<ResultKind> resultKind_{ResultKind::Failure};
    QMutex overwriteMutex_;
    QWaitCondition overwriteCondition_;
    bool overwriteWaiting_ = false;
    int overwriteAnswer_ = -1;
};
