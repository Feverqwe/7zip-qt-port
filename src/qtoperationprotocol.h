// Versioned, framed local messages for one independent GUI operation.
#pragma once
#include <QByteArray>
#include <QIODevice>
#include <QMetaType>
#include <QStringList>

struct QtOperationResult {
    enum class Outcome {
        Success,
        Warning,
        Cancelled,
        Failure,
        LaunchFailed,
        ProtocolFailed,
        Crashed
    };
    Outcome Status = Outcome::Failure;
    int ExitCode = 2;
    QString Error;
    QStringList ChangedDirectories;
};
Q_DECLARE_METATYPE(QtOperationResult)

struct QtOperationMessage {
    enum class Kind { Ready, Job, Accepted, Outputs, Result, Rejected };
    enum class Command { Test, Extract };
    Kind Type = Kind::Ready;
    Command Operation = Command::Test;
    QStringList Paths;
    bool YesToAll = false;
    QString Destination;
    bool ShowDialog = true;
    bool ElimDup = false;
    quint32 WriteZone = quint32(-1);
    QtOperationResult Result;
};

bool EncodeOperationMessage(const QtOperationMessage &message, QByteArray &frame, QString &error);

class QtOperationMessageReader final {
  public:
    enum class State { Pending, Message, Error };
    State read(QIODevice &input, QtOperationMessage &message, QString &error);

  private:
    QByteArray header_, body_;
    quint32 size_ = 0;
    bool failed_ = false;
};
