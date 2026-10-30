#pragma once
#include "rep_nativeclient_merged.h"

struct QtNativeMountParameters {
    quint16 Port = 0;
    QString Share, User, Password, Owner, Directory;
};
QString QtNativeClientOwnerIdentity();

class QtNativeClientControl final : public NativeClientSource {
    Q_OBJECT
    const QtNativeMountParameters parameters_;
    bool connected_ = false, recovery_ = false, force_ = false;

  public:
    explicit QtNativeClientControl(QtNativeMountParameters parameters);
    quint16 port() const override;
    QString share() const override;
    QString user() const override;
    QString password() const override;
    QString owner() const override;
    QString directory() const override;
    bool recovery() const override;
    bool force() const override;
    bool ready() override;
    bool connected() const;
    void prepareRecovery(bool force);
    bool mounted(QString path, QString error) override;
    bool unmounted(QString error) override;
  signals:
    void mountReported(QString path, QString error);
    void unmountReported(QString error);
};
