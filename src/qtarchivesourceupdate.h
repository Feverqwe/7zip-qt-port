// File-only engine boundary for the native UpdateOneFile callback.
#pragma once
#include "7zip/Archive/IArchive.h"
#include <QString>
class ArchiveBackend;
struct CompressionOptions;
class CProgressSync;
HRESULT UpdateArchiveSourceStream(ArchiveBackend &backend, const QString &archive,
                                  const QString &format, ISequentialOutStream *output,
                                  IInArchive *metadata, UInt32 count,
                                  IArchiveUpdateCallback *callback, CProgressSync *sync,
                                  const CompressionOptions *options = nullptr);
