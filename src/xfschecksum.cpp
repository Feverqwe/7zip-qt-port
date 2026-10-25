// XFS metadata integration around the shared LGPL 7-Zip CRC32C primitive.
#include "CPP/7zip/Archive/VhdxCrc.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

// The caller owns its metadata buffer exclusively. Restore the recorded CRC
// before returning so parsing sees exactly the original on-disk bytes.
extern "C" int xfs_metadata_checksum(void *data, size_t size, size_t offset) {
    if (offset > size || size - offset < 4)
        return 0;
    auto *p = static_cast<unsigned char *>(data);
    const uint32_t recorded = uint32_t(p[offset]) | uint32_t(p[offset + 1]) << 8 |
                              uint32_t(p[offset + 2]) << 16 | uint32_t(p[offset + 3]) << 24;
    unsigned char saved[4];
    std::memcpy(saved, p + offset, 4);
    std::memset(p + offset, 0, 4);
    const auto crc = sevenzip_crc32c_update(0xffffffff, data, size) ^ 0xffffffff;
    std::memcpy(p + offset, saved, 4);
    return crc == recorded;
}
