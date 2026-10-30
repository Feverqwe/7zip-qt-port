#pragma once
#include "mount/qtnativeclientcontrol.h"
#include <QObject>
#include <memory>

// Elevated OS-client broker. It never loads archive readers or SMB server code.
class QtNativeMountJob final : public QObject {
    Q_OBJECT
    struct State;
    std::unique_ptr<State> state_;

  public:
    explicit QtNativeMountJob(QObject *parent = nullptr);
    ~QtNativeMountJob() override;
    void mount(const QtNativeMountParameters &parameters, const QUrl &endpoint);
    void recover(const QtNativeMountParameters &parameters, const QUrl &endpoint, bool force);
    void unmount(bool force);
    bool mounted() const;
    bool busy() const;
  signals:
    void mountFinished(QString path, QString error);
    void unmountFinished(QString error);
};
