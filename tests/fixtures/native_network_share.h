#pragma once

#include <QByteArray>
#include <QString>
#include <QTemporaryDir>

// Private local SMB share, restricted to the current runner account.
class NativeNetworkShare final {
    QTemporaryDir directory_;
    QString name_;
    bool shared_ = false;

  public:
    NativeNetworkShare();
    ~NativeNetworkShare();
    NativeNetworkShare(const NativeNetworkShare &) = delete;
    NativeNetworkShare &operator=(const NativeNetworkShare &) = delete;
    QString create();
    unsigned long remove();
    const QString &name() const { return name_; }
    QString server() const;
    QString sharePath() const;
    QString filePath() const;
    static QString fileName();
    static QByteArray payload();
};
