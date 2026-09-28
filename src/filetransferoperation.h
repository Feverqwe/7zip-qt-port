#pragma once

#include <QStringList>
#include <QThread>
#include <atomic>

class FileTransferOperation : public QThread {
    Q_OBJECT

  public:
    FileTransferOperation(QStringList sources, QString destination, bool move,
                          QObject *parent = nullptr);
    void cancel();

  signals:
    void currentFile(const QString &path);
    void advanced(qint64 bytes);
    void result(bool success, const QString &message);

  protected:
    void run() override;

  private:
    bool copyEntry(const QString &source, const QString &target, QString *error);
    bool validateEntry(const QString &source, QString *error);
    static void removeEntry(const QString &path);

    QStringList sources_;
    QString destination_;
    bool move_;
    std::atomic_bool canceled_{false};
};
