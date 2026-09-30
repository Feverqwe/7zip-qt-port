#pragma once

#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QWaitCondition>
#include <atomic>

class FileTransferOperation : public QThread {
    Q_OBJECT

  public:
    FileTransferOperation(QStringList sources, QString destination, bool move,
                          QObject *parent = nullptr);
    void cancel();
    void submitOverwriteAnswer(int answer);
    bool isCanceled() const { return canceled_; }

  signals:
    void currentFile(const QString &path);
    void advanced(qint64 bytes);
    void result(bool success, const QString &message);
    void overwriteRequested(const QString &existing, const QString &incoming);
    void dismissOverwriteRequested();

  protected:
    void run() override;

  private:
    bool copyEntry(const QString &source, const QString &target, QString *error);
    bool validateEntry(const QString &source, QString *error);
    static void removeEntry(const QString &path);
    int askOverwrite(const QString &existing, const QString &incoming);

    QStringList sources_;
    QString destination_;
    bool move_;
    std::atomic_bool canceled_{false};
    QMutex overwriteMutex_;
    QWaitCondition overwriteCondition_;
    bool overwriteWaiting_ = false;
    int overwriteAnswer_ = -1;
};
