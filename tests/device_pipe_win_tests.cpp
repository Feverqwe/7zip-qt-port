#include "Common/Common0.h"
#include "Windows/Handle.h"
#include "devicepipe_win.h"
#include <QScopeGuard>
#include <QUuid>
#include <QtTest>
#include <future>

namespace {
bool Connect(HANDLE server, OVERLAPPED &operation) {
    if (ConnectNamedPipe(server, &operation))
        return true;
    const DWORD error = GetLastError();
    if (error == ERROR_PIPE_CONNECTED)
        return true;
    if (error != ERROR_IO_PENDING)
        return false;
    DWORD bytes = 0;
    if (WaitForSingleObject(operation.hEvent, 3000) == WAIT_OBJECT_0)
        return GetOverlappedResult(server, &operation, &bytes, FALSE);
    CancelIoEx(server, &operation);
    GetOverlappedResult(server, &operation, &bytes, TRUE);
    return false;
}

QByteArray Payload(quint64 offset, quint32 size) {
    QByteArray data(size, Qt::Uninitialized);
    for (quint32 i = 0; i < size; ++i)
        data[i] = char((offset + i) % 251);
    return data;
}
} // namespace

class DevicePipeWinTests final : public QObject {
    Q_OBJECT

  private slots:
    void readsAndReportsPeerErrors_data() {
        QTest::addColumn<QString>("mode");
        for (const QString &mode : {"success", "failure", "oversize", "disconnect"})
            QTest::newRow(qPrintable(mode)) << mode;
    }
    void readsAndReportsPeerErrors() {
        QFETCH(QString, mode);
        const auto name = (QStringLiteral("\\\\.\\pipe\\sevenzip-device-test-") +
                           QUuid::createUuid().toString(QUuid::Id128))
                              .toStdWString();
        NWindows::CHandle server, event;
        HANDLE raw = CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                      PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1,
                                      65536, 65536, 0, nullptr);
        QVERIFY(raw != INVALID_HANDLE_VALUE);
        server.Attach(raw);
        event.Attach(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        QVERIFY(event.IsCreated());
        auto peer = std::async(std::launch::async, [&] {
            HANDLE client = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                        OPEN_EXISTING, 0, nullptr);
            if (client == INVALID_HANDLE_VALUE)
                return false;
            NWindows::CHandle owner;
            owner.Attach(client);
            WindowsDeviceRequest request;
            if (WindowsDeviceTransfer(client, &request, sizeof(request), false, false) ||
                request.command != WindowsDeviceCommand::Read)
                return false;
            if (mode == "disconnect")
                return true;
            WindowsDeviceReply reply;
            reply.error = mode == "failure" ? ERROR_READ_FAULT : ERROR_SUCCESS;
            reply.size = mode == "oversize"  ? request.size + 1
                         : mode == "failure" ? 0
                                             : request.size;
            if (WindowsDeviceTransfer(client, &reply, sizeof(reply), true, false))
                return false;
            if (mode == "success") {
                auto bytes = Payload(request.offset, request.size);
                return !WindowsDeviceTransfer(client, bytes.data(), request.size, true, false);
            }
            return true;
        });
        const auto release = qScopeGuard([&] { server.Close(); });
        OVERLAPPED connection{};
        connection.hEvent = event;
        QVERIFY(Connect(server, connection));
        auto reader = AdoptWindowsDevicePipe(server.Detach(), nullptr);
        QVERIFY(reader);
        QByteArray output(8192, Qt::Uninitialized);
        quint32 count = 0;
        const auto result = reader->readAt(1234, output.data(), quint32(output.size()), count);
        if (mode == "success") {
            QCOMPARE(result, qint32(0));
            QCOMPARE(count, quint32(output.size()));
            QCOMPARE(output, Payload(1234, quint32(output.size())));
        } else {
            QVERIFY(result != 0);
            QCOMPARE(count, quint32(0));
        }
        reader.reset();
        QVERIFY(peer.get());
    }
    void cancellationInterruptsPendingRead() {
        const auto name = (QStringLiteral("\\\\.\\pipe\\sevenzip-device-cancel-") +
                           QUuid::createUuid().toString(QUuid::Id128))
                              .toStdWString();
        NWindows::CHandle server, client, event;
        HANDLE raw = CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                      PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1,
                                      4096, 4096, 0, nullptr);
        QVERIFY(raw != INVALID_HANDLE_VALUE);
        server.Attach(raw);
        event.Attach(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        QVERIFY(event.IsCreated());
        auto connecting = std::async(std::launch::async, [&] {
            return CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                               OPEN_EXISTING, 0, nullptr);
        });
        OVERLAPPED connection{};
        connection.hEvent = event;
        QVERIFY(Connect(server, connection));
        raw = connecting.get();
        QVERIFY(raw != INVALID_HANDLE_VALUE);
        client.Attach(raw);
        char byte = 0;
        QCOMPARE(WindowsDeviceTransfer(server, &byte, 1, false, true, [] { return true; }),
                 DWORD(ERROR_CANCELLED));
    }
};

QTEST_GUILESS_MAIN(DevicePipeWinTests)
#include "device_pipe_win_tests.moc"
