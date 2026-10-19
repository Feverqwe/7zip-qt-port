#pragma once

#include <QList>
#include <QString>
#include <functional>
#include <memory>

struct IInStream;

struct ExternalDevice {
    QString id;
    QString name;
    QString identity;
    quint64 size = 0;
    quint32 sectorSize = 0;
    bool mounted = false;
};

enum class DeviceAccessStatus { Ready, Cancelled, PermissionRequired, Busy, Failed };

// Native readers own only an already authorized read-only OS handle. Requests
// use sector-aligned positions, lengths and buffers. No filesystem parsing or
// destination-file operations run with elevated privileges.
class DeviceReadHandle {
  public:
    virtual ~DeviceReadHandle() = default;
    virtual qint32 readAt(quint64 offset, void *data, quint32 size, quint32 &read) = 0;
};

struct DeviceAccessResult {
    DeviceAccessStatus status = DeviceAccessStatus::Failed;
    QString error;
    std::shared_ptr<DeviceReadHandle> handle;
};

QList<ExternalDevice> EnumerateExternalDevices(QString &error);
DeviceAccessResult OpenExternalDevice(const ExternalDevice &device, bool authorize,
                                      const std::function<bool()> &cancelled = {});

// Separates authorized native device sources from nested archive streams, which
// continue using the existing spool-to-file transport.
class DirectDeviceInput {
  public:
    virtual ~DirectDeviceInput() = default;
    virtual quint64 deviceInputSize() const = 0;
    virtual bool ignoresDeviceJournal() const { return false; }
};

qint32 CreateDeviceInput(const ExternalDevice &device, std::shared_ptr<DeviceReadHandle> handle,
                         IInStream **stream);
qint32 CreateDevicePartitionInput(IInStream *device, quint64 offset, quint64 size,
                                  IInStream **stream);
// A separate, bounded lease for an explicitly selected read-without-replay
// session. Does not modify the original stream's policy or the device.
qint32 CreateDeviceInputIgnoringJournal(IInStream *device, IInStream **stream);
