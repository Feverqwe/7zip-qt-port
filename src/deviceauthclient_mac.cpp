#include "deviceaccess_mac.h"
#include "deviceaccess_posix.h"
#include "deviceauthsocket_mac.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
class Descriptor {
  public:
    int value = -1;
    ~Descriptor() {
        if (value >= 0)
            ::close(value);
    }
};

bool Listen(const QString &socketPath, Descriptor &server) {
    const QByteArray path = QFile::encodeName(socketPath);
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (path.size() >= qsizetype(sizeof(address.sun_path)))
        return false;
    std::memcpy(address.sun_path, path.constData(), size_t(path.size()) + 1);
    server.value = ::socket(AF_UNIX, SOCK_STREAM, 0);
    return server.value >= 0 && ::fcntl(server.value, F_SETFD, FD_CLOEXEC) == 0 &&
           ::bind(server.value, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0 &&
           ::listen(server.value, 1) == 0;
}

void Stop(QProcess &launcher) {
    if (!launcher.waitForFinished(5000)) {
        launcher.kill();
        launcher.waitForFinished(5000);
    }
}

int WaitForPeer(int server, QProcess &launcher, const std::function<bool()> &cancelled) {
    for (;;) {
        if (cancelled && cancelled())
            return -1;
        pollfd pending{server, POLLIN, 0};
        const int ready = ::poll(&pending, 1, 50);
        if (ready < 0 && errno != EINTR)
            return -1;
        if (ready > 0 && (pending.revents & POLLIN)) {
            const int peer = ::accept(server, nullptr, nullptr);
            uid_t user = uid_t(-1);
            gid_t group = gid_t(-1);
            if (peer >= 0 && ::getpeereid(peer, &user, &group) == 0 && user == 0)
                return peer;
            if (peer >= 0)
                ::close(peer);
        }
        launcher.waitForFinished(0);
        if (launcher.state() == QProcess::NotRunning)
            return -1;
    }
}

DeviceAccessResult Receive(int peer, const std::function<bool()> &cancelled) {
    DeviceAccessResult result;
    pollfd pending{peer, POLLIN, 0};
    int ready = 0;
    for (unsigned attempt = 0; attempt < 200 && ready <= 0; ++attempt) {
        if (cancelled && cancelled()) {
            result.status = DeviceAccessStatus::Cancelled;
            return result;
        }
        ready = ::poll(&pending, 1, 50);
        if (ready < 0 && errno != EINTR)
            break;
    }
    if (ready <= 0) {
        result.error =
            QCoreApplication::translate("deviceaccess", "The device helper did not respond.");
        return result;
    }
    MacDeviceReply reply{};
    iovec bytes{&reply, sizeof(reply)};
    alignas(cmsghdr) char control[CMSG_SPACE(sizeof(int))]{};
    msghdr message{};
    message.msg_iov = &bytes;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);
    const ssize_t received = ::recvmsg(peer, &message, 0);
    int descriptor = -1;
    for (cmsghdr *header = CMSG_FIRSTHDR(&message); header; header = CMSG_NXTHDR(&message, header))
        if (header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_RIGHTS &&
            header->cmsg_len == CMSG_LEN(sizeof(int)))
            std::memcpy(&descriptor, CMSG_DATA(header), sizeof(descriptor));
    if (received != ssize_t(sizeof(reply)) || (message.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) ||
        reply.error || descriptor < 0) {
        if (descriptor >= 0)
            ::close(descriptor);
        result.status =
            reply.error == EBUSY ? DeviceAccessStatus::Busy : DeviceAccessStatus::Failed;
        result.error = reply.error
                           ? QString::fromLocal8Bit(std::strerror(reply.error))
                           : QCoreApplication::translate(
                                 "deviceaccess", "The device helper returned an invalid response.");
        return result;
    }
    result.handle = AdoptReadOnlyDeviceDescriptor(descriptor);
    result.status = result.handle ? DeviceAccessStatus::Ready : DeviceAccessStatus::Failed;
    return result;
}
} // namespace

DeviceAccessResult MacAuthorizeExternalDevice(const ExternalDevice &device,
                                              const std::function<bool()> &cancelled) {
    DeviceAccessResult result;
    const QString helper =
        QDir(QCoreApplication::applicationDirPath()).filePath("SevenzipDeviceReader");
    if (!QFileInfo(helper).isExecutable()) {
        result.error = QCoreApplication::translate("deviceaccess",
                                                   "The Sevenzip device helper is unavailable.");
        return result;
    }
    // A short random directory keeps sockaddr_un within its native path limit.
    // QTemporaryDir creates it private to this user; peers must additionally be root.
    QTemporaryDir directory(QStringLiteral("/tmp/sevenzip-device-XXXXXX"));
    Descriptor server;
    const QString socketPath = directory.filePath("read");
    if (!directory.isValid() || !Listen(socketPath, server)) {
        result.error =
            QCoreApplication::translate("deviceaccess", "Could not prepare device authorization.");
        return result;
    }
    // AppleScript quotes every argument, including the executable. The program
    // contains no interpolated paths or user text and installs no system service.
    const QString script = QStringLiteral(
        "on run argv\n"
        "set commandText to \"\"\n"
        "repeat with argument in argv\n"
        "set commandText to commandText & quoted form of (argument as text) & \" \"\n"
        "end repeat\n"
        "do shell script commandText with administrator privileges\n"
        "end run");
    QProcess launcher;
    launcher.start(QStringLiteral("/usr/bin/osascript"),
                   {"-e", script, "--", helper, "--device-socket", socketPath, device.id,
                    device.identity, QString::number(device.size),
                    QString::number(device.sectorSize), QString::number(::getuid())});
    if (!launcher.waitForStarted(5000)) {
        result.error = launcher.errorString();
        return result;
    }
    Descriptor peer;
    peer.value = WaitForPeer(server.value, launcher, cancelled);
    if (peer.value >= 0)
        result = Receive(peer.value, cancelled);
    else {
        const QString error = QString::fromUtf8(launcher.readAllStandardError()).trimmed();
        result.error = error;
        result.status =
            error.contains("(-128)") ? DeviceAccessStatus::Cancelled : DeviceAccessStatus::Failed;
    }
    if (cancelled && cancelled()) {
        launcher.kill();
        result.handle.reset();
        result.status = DeviceAccessStatus::Cancelled;
    }
    Stop(launcher);
    return result;
}
