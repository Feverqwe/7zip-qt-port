#pragma once
#include <QStringList>
#include <QThread>
#include <atomic>

class FileDeleteOperation : public QThread {
    Q_OBJECT
  public:
    FileDeleteOperation(QString directory, QStringList paths, QObject *parent = nullptr);
    void cancel();
  signals:
    void currentFile(const QString &path);
    void advanced(int completed, int total);
    void result(bool success, const QString &message);

  protected:
    void run() override;

  private:
    QString directory_;
    QStringList paths_;
    std::atomic_bool canceled_{false};
};
