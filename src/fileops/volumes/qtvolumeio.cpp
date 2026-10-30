#include "fileops/volumes/qtvolumeio.h"

bool QtVolumeRead(NWindows::NFile::NIO::CInFile &file, void *data, UInt32 size, UInt32 &processed) {
#ifdef _WIN32
    return file.Read(data, size, processed);
#else
    size_t actual = 0;
    const bool result = file.ReadFull(data, size, actual);
    processed = UInt32(actual);
    return result;
#endif
}
bool QtVolumeWrite(NWindows::NFile::NIO::COutFile &file, const void *data, UInt32 size,
                   UInt32 &processed) {
#ifdef _WIN32
    return file.Write(data, size, processed);
#else
    size_t actual = 0;
    const ssize_t result = file.write_full(data, size, actual);
    processed = UInt32(actual);
    return result != -1;
#endif
}
void QtVolumeSeekToBegin(NWindows::NFile::NIO::COutFile &file) {
#ifdef _WIN32
    file.SeekToBegin();
#else
    file.seekToBegin();
#endif
}
