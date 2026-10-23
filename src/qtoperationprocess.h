// Process lifetime boundary for independent 7zG operations.
#pragma once
#include <QString>
#include <memory>

class QtOperationProcess final {
  public:
    enum class State { Idle, Running, Exited, Crashed, ObservationFailed };
    struct Result {
        State Status = State::Idle;
        quint32 Code = 0;
        QString Error;
    };
    QtOperationProcess();
    ~QtOperationProcess(); // Closes observation; never terminates the child.
    QtOperationProcess(const QtOperationProcess &) = delete;
    QtOperationProcess &operator=(const QtOperationProcess &) = delete;
    bool start(const QString &program, const QString &channel, QString &error);
    Result poll();
    quint64 processId() const;

  private:
    struct Native;
    std::unique_ptr<Native> native_;
    Result result_;
};
