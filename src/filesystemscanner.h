#pragma once

#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include <QFileInfo>
#include <QList>
#include <QObject>
class FilesystemFolderLoader;

class FilesystemScanner : public QObject {
    Q_OBJECT

  public:
    explicit FilesystemScanner(QObject *parent = nullptr);
    ~FilesystemScanner() override;

    void start(const QString &directory);
    void cancel();
    void cancelAndWait();
    bool isRunning() const;

  signals:
    // All signals are dispatched on the GUI thread. Retain folderReady's
    // borrowed pointer through CMyComPtr when it must outlive the signal.
    void folderReady(IFolderFolder *folder);
    void batchReady(const QFileInfoList &files, quint32 firstSourceIndex);
    void advanced(quint64 count);
    void failed(qint32 result);
    void completed();

  private:
    void scanBatch(quint64 generation);
    void loaderFinished(FilesystemFolderLoader *loader, quint64 generation);

    quint64 generation_ = 0;
    FilesystemFolderLoader *loader_ = nullptr;
    QList<FilesystemFolderLoader *> loaders_;
    CMyComPtr<IFolderFolder> folder_;
    QString directory_;
    quint32 nextIndex_ = 0;
    bool presenting_ = false;
};
