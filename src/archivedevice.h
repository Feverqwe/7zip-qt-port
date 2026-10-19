#pragma once

#include "7zip/IStream.h"
#include "Common/MyCom.h"
#include <QList>
#include <QString>
#include <memory>

class ArchiveCodecCatalog;
class ArchiveNativeOpen;
struct IOpenCallbackUI;

struct ArchiveDevicePartition {
    QString name;
    QString type;
    UInt64 offset = 0;
    UInt64 size = 0;
};

// A read-only, seekable device lease. OS discovery/authorization supplies the
// stream and its verified size; original 7-Zip handlers own all format parsing.
// Calls belong to the opening/extraction worker, never the GUI event loop.
class ArchiveDevice final {
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;
    CMyComPtr<IInStream> input_;
    QList<ArchiveDevicePartition> partitions_;
    QString name_;

  public:
    explicit ArchiveDevice(std::shared_ptr<const ArchiveCodecCatalog> catalog);
    HRESULT inspect(IInStream *input, UInt64 size, const QString &name, IOpenCallbackUI *callback);
    const QList<ArchiveDevicePartition> &partitions() const { return partitions_; }
    HRESULT partitionInput(int index, IInStream **stream) const;
    HRESULT openPartition(int index, IOpenCallbackUI *callback,
                          std::shared_ptr<ArchiveNativeOpen> &reader) const;
};

HRESULT OpenDeviceFilesystem(const std::shared_ptr<const ArchiveCodecCatalog> &catalog,
                             IInStream *input, const QString &name, IOpenCallbackUI *callback,
                             std::shared_ptr<ArchiveNativeOpen> &reader);
