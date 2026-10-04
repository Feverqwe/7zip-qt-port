#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include <QObject>
#include <QTimer>
#include <functional>

// GUI scheduling only. The source folder still owns items and real indices;
// this retains it until all rows have been presented or navigation cancels.
class QtFolderProjection final : public QObject {
  public:
    explicit QtFolderProjection(QObject *parent = nullptr);
    void start(IFolderFolder *folder, quint32 count, std::function<void(quint32)> append,
               std::function<void()> completed);
    void cancel();
    bool active() const { return bool(folder_); }

  private:
    void batch();
    CMyComPtr<IFolderFolder> folder_;
    QTimer timer_{this};
    quint64 generation_ = 0;
    quint32 next_ = 0;
    quint32 count_ = 0;
    std::function<void(quint32)> append_;
    std::function<void()> completed_;
};
