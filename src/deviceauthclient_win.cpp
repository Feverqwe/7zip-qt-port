#include "Common/Common0.h"
#include "Windows/Handle.h"
#include "deviceaccess_win.h"
#include "devicepipe_win.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QUuid>
#include <sddl.h>
#include <shellapi.h>

namespace {
QString UserSid() {
    HANDLE raw = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &raw))
        return {};
    NWindows::CHandle token;
    token.Attach(raw);
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    QByteArray data(size, Qt::Uninitialized);
    if (!size || !GetTokenInformation(token, TokenUser, data.data(), size, &size))
        return {};
    LPWSTR text = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(data.data())->User.Sid, &text))
        return {};
    const QString result = QString::fromWCharArray(text);
    LocalFree(text);
    return result;
}

HANDLE MakePipe(const QString &name) {
    const QString sid = UserSid();
    if (sid.isEmpty())
        return INVALID_HANDLE_VALUE;
    const auto dacl =
        (QStringLiteral("D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;") + sid + ")").toStdWString();
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(dacl.c_str(), SDDL_REVISION_1,
                                                              &descriptor, nullptr))
        return INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES security{sizeof(security), descriptor, FALSE};
    HANDLE result = CreateNamedPipeW(
        name.toStdWString().c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 65536,
        65536, 0, &security);
    LocalFree(descriptor);
    return result;
}

DWORD LaunchAndConnect(HANDLE pipe, const QString &pipeName, NWindows::CHandle &helper,
                       const std::function<bool()> &cancelled) {
    NWindows::CHandle event;
    event.Attach(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event.IsCreated())
        return GetLastError();
    OVERLAPPED connection{};
    connection.hEvent = event;
    const BOOL connected = ConnectNamedPipe(pipe, &connection);
    const DWORD state = connected ? ERROR_SUCCESS : GetLastError();
    if (state != ERROR_SUCCESS && state != ERROR_PIPE_CONNECTED && state != ERROR_IO_PENDING)
        return state;
    const bool pending = state == ERROR_IO_PENDING;
    const auto cancelConnection = qScopeGuard([&] {
        if (pending) {
            DWORD count = 0;
            CancelIoEx(pipe, &connection);
            GetOverlappedResult(pipe, &connection, &count, TRUE);
        }
    });
    const QString executable =
        QDir(QCoreApplication::applicationDirPath()).filePath("SevenzipDeviceReader.exe");
    const auto file = QDir::toNativeSeparators(executable).toStdWString();
    const auto parameters =
        ("--device-pipe " + pipeName + ' ' + QString::number(GetCurrentProcessId())).toStdWString();
    if (!QFileInfo(executable).isExecutable())
        return ERROR_FILE_NOT_FOUND;
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const auto uninitialize = qScopeGuard([&] {
        if (SUCCEEDED(initialized))
            CoUninitialize();
    });
    SHELLEXECUTEINFOW launch{};
    launch.cbSize = sizeof(launch);
    launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    launch.lpVerb = L"runas";
    launch.lpFile = file.c_str();
    launch.lpParameters = parameters.c_str();
    launch.nShow = SW_HIDE;
    if (!ShellExecuteExW(&launch))
        return GetLastError();
    helper.Attach(launch.hProcess);
    for (unsigned attempt = 0; pending && attempt < 200; ++attempt) {
        if (cancelled && cancelled())
            return ERROR_CANCELLED;
        const DWORD ready = WaitForSingleObject(event, 50);
        if (ready == WAIT_OBJECT_0)
            break;
        if (ready != WAIT_TIMEOUT || WaitForSingleObject(helper, 0) == WAIT_OBJECT_0)
            return ERROR_BROKEN_PIPE;
    }
    if (pending) {
        DWORD count = 0;
        if (!GetOverlappedResult(pipe, &connection, &count, FALSE))
            return GetLastError() == ERROR_IO_INCOMPLETE ? ERROR_TIMEOUT : GetLastError();
    }
    ULONG peer = 0;
    if (!GetNamedPipeClientProcessId(pipe, &peer) || !peer || peer != GetProcessId(helper))
        return ERROR_ACCESS_DENIED;
    return ERROR_SUCCESS;
}

DWORD SelectDevice(HANDLE pipe, const ExternalDevice &device,
                   const std::function<bool()> &cancelled) {
    QJsonObject choice{{"id", device.id},
                       {"identity", device.identity},
                       {"size", QString::number(device.size)},
                       {"sector", int(device.sectorSize)}};
    QByteArray bytes = QJsonDocument(choice).toJson(QJsonDocument::Compact);
    if (bytes.isEmpty() || bytes.size() > 65536)
        return ERROR_INVALID_DATA;
    WindowsDeviceRequest request{WindowsDeviceCommand::Open, quint32(bytes.size()), 0};
    WindowsDeviceReply reply;
    DWORD error = WindowsDeviceTransfer(pipe, &request, sizeof(request), true, true, cancelled);
    if (!error)
        error =
            WindowsDeviceTransfer(pipe, bytes.data(), DWORD(bytes.size()), true, true, cancelled);
    if (!error)
        error = WindowsDeviceTransfer(pipe, &reply, sizeof(reply), false, true, cancelled);
    return error ? error : reply.error;
}
} // namespace

DeviceAccessResult WindowsAuthorizeExternalDevice(const ExternalDevice &device,
                                                  const std::function<bool()> &cancelled) {
    DeviceAccessResult result;
    if (cancelled && cancelled()) {
        result.status = DeviceAccessStatus::Cancelled;
        return result;
    }
    const QString name = QStringLiteral("\\\\.\\pipe\\sevenzip-device-") +
                         QUuid::createUuid().toString(QUuid::Id128);
    HANDLE raw = MakePipe(name);
    if (raw == INVALID_HANDLE_VALUE) {
        result.error =
            QCoreApplication::translate("deviceaccess", "Could not prepare device authorization.");
        return result;
    }
    NWindows::CHandle pipe, helper;
    pipe.Attach(raw);
    DWORD error = LaunchAndConnect(pipe, name, helper, cancelled);
    if (!error && cancelled && cancelled())
        error = ERROR_CANCELLED;
    if (!error)
        error = SelectDevice(pipe, device, cancelled);
    if (!error && cancelled && cancelled())
        error = ERROR_CANCELLED;
    if (error) {
        pipe.Close();
        if (helper.IsCreated() && WaitForSingleObject(helper, 500) == WAIT_TIMEOUT)
            TerminateProcess(helper, error);
        result.status = error == ERROR_CANCELLED ? DeviceAccessStatus::Cancelled
                        : error == ERROR_BUSY || error == ERROR_SHARING_VIOLATION
                            ? DeviceAccessStatus::Busy
                            : DeviceAccessStatus::Failed;
        result.error = QCoreApplication::translate("deviceaccess",
                                                   "Could not authorize access to this device.");
        return result;
    }
    result.handle = AdoptWindowsDevicePipe(pipe.Detach(), helper.Detach());
    result.status = result.handle ? DeviceAccessStatus::Ready : DeviceAccessStatus::Failed;
    return result;
}
