#pragma once
#include "qtoperationprotocol.h"
#include <QObject>
#include <memory>

// One app-owned observer per operation; closing a manager window drops only its
// signal receivers. Closing this observer never terminates independent work.
class QtOperationLauncher final : public QObject {
    Q_OBJECT
  public:
    explicit QtOperationLauncher(QObject *parent = nullptr);
    ~QtOperationLauncher() override;
    static QString GuiProgram();
    static QtOperationLauncher *Launch(const QString &program, const QtOperationMessage &job,
                                       QString &error);
    bool start(const QString &program, const QtOperationMessage &job, QString &error);
    quint64 processId() const;
  signals:
    void accepted();
    void finished(const QtOperationResult &result);

  private:
    void acceptConnection();
    void readMessages();
    void observe();
    void finish(QtOperationResult result);
    void protocolFailure(const QString &error);
    struct State;
    std::unique_ptr<State> state_;
};
