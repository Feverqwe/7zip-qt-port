#pragma once

#include <QFile>
#include <QString>

// Platform boundary for the source panel's recycle-bin branch. QFile delegates
// to the native Trash/Recycle Bin service on each supported desktop OS.
class IQtTrashService {
  public:
    virtual ~IQtTrashService() = default;
    virtual bool MoveToTrash(const QString &path) = 0;
};

class QtNativeTrashService final : public IQtTrashService {
  public:
    bool MoveToTrash(const QString &path) override { return QFile::moveToTrash(path); }
};
