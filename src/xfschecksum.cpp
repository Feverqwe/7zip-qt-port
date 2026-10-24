// CRC32C bodies adapted from 7-Zip VhdxHandler.cpp, Igor Pavlov.
// LGPL-2.1-or-later; selected symbols: docs/xfs-sevenzip-baseline.json.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
constexpr std::array<uint32_t, 256> Crc32c_GenerateTable() {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t r = i;
        for (unsigned j = 0; j < 8; j++)
            r = (r >> 1) ^ (0x82f63b78 & (uint32_t(0) - (r & 1)));
        table[i] = r;
    }
    return table;
}
constexpr auto crcTable = Crc32c_GenerateTable();
uint32_t CrcUpdateT1_vhdx(uint32_t v, const void *data, size_t size) {
    const auto *p = static_cast<const unsigned char *>(data);
    const auto *end = p + size;
    for (; p != end; p++)
        v = crcTable[(v ^ *p) & 0xff] ^ (v >> 8);
    return v;
}
} // namespace

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
    const auto crc = CrcUpdateT1_vhdx(0xffffffff, data, size) ^ 0xffffffff;
    std::memcpy(p + offset, saved, 4);
    return crc == recorded;
}
