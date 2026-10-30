// One-shot privileged boundary, launched only after macOS administrator consent.
// This executable links native device discovery and QtCore, no archive handlers.
#include "devices/deviceaccess_mac.h"
#include "devices/deviceauthsocket_mac.h"
#include <QCoreApplication>
#include <QFile>
#include <cerrno>
#include <cstring>
#include <limits>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const QStringList arguments = app.arguments();
    if (::geteuid() != 0 || arguments.size() != 8 || arguments[1] != "--device-socket")
        return 1;
    bool sizeOk = false, sectorOk = false, userOk = false;
    ExternalDevice device;
    device.id = arguments[3];
    device.identity = arguments[4];
    device.size = arguments[5].toULongLong(&sizeOk);
    const quint64 sector = arguments[6].toULongLong(&sectorOk);
    const quint64 expectedUser = arguments[7].toULongLong(&userOk);
    if (!sizeOk || !sectorOk || !userOk || sector > std::numeric_limits<quint32>::max() ||
        expectedUser > std::numeric_limits<uid_t>::max() || device.id.size() > 64 ||
        device.identity.size() > 64)
        return 1;
    device.sectorSize = quint32(sector);
    const QByteArray path = QFile::encodeName(arguments[2]);
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (path.isEmpty() || path.size() >= qsizetype(sizeof(address.sun_path)))
        return 1;
    std::memcpy(address.sun_path, path.constData(), size_t(path.size()) + 1);
    const int socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket < 0)
        return 1;
    uid_t user = 0;
    gid_t group = 0;
    if (::connect(socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 ||
        ::getpeereid(socket, &user, &group) != 0 || user != uid_t(expectedUser)) {
        ::close(socket);
        return 1;
    }
    QString error;
    const int descriptor = MacOpenExternalDeviceDescriptor(device, error);
    MacDeviceReply reply;
    reply.error = descriptor < 0 ? (errno ? errno : EIO) : 0;
    iovec bytes{&reply, sizeof(reply)};
    msghdr message{};
    message.msg_iov = &bytes;
    message.msg_iovlen = 1;
    alignas(cmsghdr) char control[CMSG_SPACE(sizeof(int))]{};
    if (descriptor >= 0) {
        message.msg_control = control;
        message.msg_controllen = sizeof(control);
        cmsghdr *header = CMSG_FIRSTHDR(&message);
        header->cmsg_level = SOL_SOCKET;
        header->cmsg_type = SCM_RIGHTS;
        header->cmsg_len = CMSG_LEN(sizeof(int));
        std::memcpy(CMSG_DATA(header), &descriptor, sizeof(descriptor));
    }
    const bool sent = ::sendmsg(socket, &message, 0) == ssize_t(sizeof(reply));
    if (descriptor >= 0)
        ::close(descriptor);
    ::close(socket);
    return sent ? 0 : 1;
}
