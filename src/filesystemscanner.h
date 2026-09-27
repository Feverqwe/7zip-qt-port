#pragma once

#include <QDir>
#include <QFileInfo>
#include <QObject>

#include <memory>

class QDirIterator;

class FilesystemScanner : public QObject {
    Q_OBJECT

  public:
    explicit FilesystemScanner(QObject *parent = nullptr);
    ~FilesystemScanner() override;

    void start(const QString &directory, QDir::Filters filters);
    void cancel();
    bool isRunning() const;

  signals:
    void batchReady(const QFileInfoList &files);
    void completed();

  private:
    void scanBatch(quint64 generation);

    quint64 generation_ = 0;
    std::unique_ptr<QDirIterator> iterator_;
};
