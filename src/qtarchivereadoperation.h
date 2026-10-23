#pragma once
#include "7zip/Archive/IArchive.h"
#include "qtworkerprogress.h"
#include <QList>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QWaitCondition>
#include <functional>
#include <memory>

class ArchiveCodecCatalog;
class ArchiveNativeOpen;
struct CArcErrorInfo;

// Joined worker around official opening and a read-only metadata/test task.
class QtArchiveReadOperation final : public QThread {
  public:
    struct ItemError {
        QString Path;
        Int32 Result;
        bool Encrypted;
    };
    struct OpenError {
        QString Path;
        UInt32 ErrorFlags;
        UInt32 WarningFlags;
        QString ErrorMessage;
        QString WarningMessage;
    };
    using Work = std::function<HRESULT(const std::shared_ptr<ArchiveNativeOpen> &,
                                       IArchiveExtractCallback *)>;
    enum class Kind { Test, List };
    QtArchiveReadOperation(QString path, QString password,
                           std::shared_ptr<const ArchiveCodecCatalog> catalog, Work work,
                           Kind kind = Kind::Test, QString format = {});
    void cancel();
    void setPaused(bool paused);
    void providePassword(const QString &password);
    HRESULT checkpoint();
    HRESULT password(BSTR *value);
    void report(quint64 completed, quint64 total, const QString &path);
    HRESULT Result = E_FAIL;
    QList<ItemError> Errors;
    QList<OpenError> OpenErrors;
    std::function<void(quint64, quint64, const QString &)> Progress;
    std::function<void()> PasswordRequired;

  private:
    void run() override;
    void recordOpenError(const QString &path, const CArcErrorInfo &error);
    // Native interfaces stay inside run and release before this module lease.
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;
    const QString path_;
    Work work_;
    const Kind kind_;
    const QString format_;
    QtWorkerProgress progress_;
    QMutex passwordMutex_;
    QWaitCondition passwordReady_;
    QString password_;
    bool passwordDefined_;
    bool passwordRequested_ = false;
};
