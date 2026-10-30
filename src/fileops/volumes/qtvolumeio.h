#pragma once
#include "Windows/FileIO.h"

// Windows FileIO's UInt32 interface and portable FileIO's size_t interface
// have different names. Both implementations belong to upstream FileIO.cpp.
bool QtVolumeRead(NWindows::NFile::NIO::CInFile &file, void *data, UInt32 size, UInt32 &processed);
bool QtVolumeWrite(NWindows::NFile::NIO::COutFile &file, const void *data, UInt32 size,
                   UInt32 &processed);
void QtVolumeSeekToBegin(NWindows::NFile::NIO::COutFile &file);
