#include "Common/Common0.h"
#include "Windows/Handle.h"
#include "devices/deviceaccess_win.h"
#include "devices/devicepipe_win.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <limits>

namespace {
bool Elevated() {
    NWindows::CHandle token;
    HANDLE raw = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &raw))
        return false;
    token.Attach(raw);
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    return GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) &&
           elevation.TokenIsElevated;
}

bool OpenDevice(HANDLE pipe, ExternalDevice &device, std::shared_ptr<DeviceReadHandle> &reader) {
    WindowsDeviceRequest request;
    if (WindowsDeviceTransfer(pipe, &request, sizeof(request), false, false) ||
        request.command != WindowsDeviceCommand::Open || !request.size || request.size > 65536)
        return false;
    QByteArray bytes(request.size, Qt::Uninitialized);
    if (WindowsDeviceTransfer(pipe, bytes.data(), DWORD(bytes.size()), false, false))
        return false;
    const auto object = QJsonDocument::fromJson(bytes).object();
    device.id = object.value("id").toString();
    device.identity = object.value("identity").toString();
    bool valid = false;
    device.size = object.value("size").toString().toULongLong(&valid);
    device.sectorSize = quint32(object.value("sector").toInt());
    WindowsDeviceReply reply;
    DWORD error = ERROR_INVALID_DATA;
    HANDLE handle = valid ? WindowsOpenExternalDevice(device, error) : INVALID_HANDLE_VALUE;
    reply.error = error;
    if (handle != INVALID_HANDLE_VALUE)
        reader = AdoptWindowsReadHandle(handle);
    if (!reader && !reply.error)
        reply.error = ERROR_INVALID_DATA;
    return !WindowsDeviceTransfer(pipe, &reply, sizeof(reply), true, false) && bool(reader);
}

void ReadRequests(HANDLE pipe, const ExternalDevice &device, DeviceReadHandle &reader) {
    WindowsDeviceRequest request;
    while (!WindowsDeviceTransfer(pipe, &request, sizeof(request), false, false)) {
        if (request.command != WindowsDeviceCommand::Read || !request.size ||
            request.size > 1024 * 1024 || request.offset > device.size ||
            request.size > device.size - request.offset || !device.sectorSize ||
            request.offset % device.sectorSize || request.size % device.sectorSize)
            return;
        QByteArray buffer(request.size + device.sectorSize, Qt::Uninitialized);
        auto *data = reinterpret_cast<void *>((quintptr(buffer.data()) + device.sectorSize - 1) &
                                              ~(quintptr(device.sectorSize) - 1));
        WindowsDeviceReply reply;
        const qint32 result = reader.readAt(request.offset, data, request.size, reply.size);
        if (result)
            reply.error = HRESULT_FACILITY(result) == FACILITY_WIN32 ? HRESULT_CODE(result)
                                                                     : ERROR_READ_FAULT;
        if (WindowsDeviceTransfer(pipe, &reply, sizeof(reply), true, false) ||
            (reply.size && WindowsDeviceTransfer(pipe, data, reply.size, true, false)))
            return;
    }
}
} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const auto arguments = app.arguments();
    if (!Elevated() || arguments.size() != 4 || arguments[1] != "--device-pipe" ||
        !arguments[2].startsWith(QStringLiteral("\\\\.\\pipe\\sevenzip-device-")))
        return 1;
    bool valid = false;
    const quint64 expectedParent = arguments[3].toULongLong(&valid);
    if (!valid || !expectedParent || expectedParent > std::numeric_limits<ULONG>::max())
        return 1;
    HANDLE raw =
        CreateFileW(arguments[2].toStdWString().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                    OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
    if (raw == INVALID_HANDLE_VALUE)
        return 1;
    NWindows::CHandle pipe;
    pipe.Attach(raw);
    ULONG parent = 0;
    if (!GetNamedPipeServerProcessId(pipe, &parent) || parent != expectedParent)
        return 1;
    ExternalDevice device;
    std::shared_ptr<DeviceReadHandle> reader;
    if (!OpenDevice(pipe, device, reader))
        return 1;
    ReadRequests(pipe, device, *reader);
    return 0;
}
