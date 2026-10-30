#include "archive/backend/archivebackend.h"
#include "archive/resources/archivedevice.h"

// Stateless worker entry, also usable by metadata-only consumers without the
// process-backed backend module. The retained catalog owns all format code.
HRESULT ArchiveBackend::inspectDevice(const std::shared_ptr<const ArchiveCodecCatalog> &catalog,
                                      IInStream *input, quint64 size, const QString &name,
                                      IOpenCallbackUI *callback,
                                      std::shared_ptr<ArchiveDevice> &source) {
    source.reset();
    auto device = std::make_shared<ArchiveDevice>(catalog);
    const HRESULT result = device->inspect(input, size, name, callback);
    if (result == S_OK)
        source = std::move(device);
    return result;
}
