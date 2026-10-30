#pragma once
#include <QObject>
#include <memory>

// Owns only the OS client operation. The caller keeps the loopback server and
// reader alive until unmountFinished reports success.
class QtSmbMountPlatform final : public QObject {
    Q_OBJECT
    struct State;
    std::unique_ptr<State> state_;
    void launchHelper();
    void helperEnded();
    void launchFailed(const QString &error);

  public:
    explicit QtSmbMountPlatform(QObject *parent = nullptr);
    ~QtSmbMountPlatform() override;
    static QString unavailableReason();
    void mount(quint16 port, const QString &share, const QString &user, const QString &password,
               const QString &directory);
    void unmount(const QString &directory, bool force);

  signals:
    void mountFinished(QString directory, QString error);
    void unmountFinished(QString error);
};
