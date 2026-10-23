// Async destination extraction through the original Common/Extract workflow.
#pragma once
#include "archivebackend.h"

class QtArchiveReadOperation;
class ArchiveNativeOpen;
struct IArchiveExtractCallback;

struct QtArchiveDirectoryExtractRequest {
    QString Destination;
    QStringList Selected;
    ExtractionOptions Options;
};

HRESULT ExtractQtArchiveDirectory(QtArchiveReadOperation &operation,
                                  const std::shared_ptr<ArchiveNativeOpen> &reader,
                                  IArchiveExtractCallback *progress);
