// CRC32C extracted from 7-Zip VhdxHandler.cpp, Igor Pavlov, LGPL-2.1-or-later.
// Original inventory unit and selected symbols: docs/upstream-baseline.json.
#include "7zip/Archive/StdAfx.h"

#include "C/7zTypes.h"
#include "formats/vhdx/VhdxCrc.h"

namespace {
// CRC-32C (Castagnoli) : reversed for poly 0x1EDC6F41
const UInt32 k_Crc32c_Poly = 0x82f63b78;

void Z7_FASTCALL Crc32c_GenerateTable(UInt32 *table) {
    for (UInt32 i = 0; i < 256; i++) {
        UInt32 r = i;
        for (unsigned j = 0; j < 8; j++)
            r = (r >> 1) ^ (k_Crc32c_Poly & ((UInt32)0 - (r & 1)));
        table[i] = r;
    }
}

struct CCrc32cTable {
    UInt32 Values[256];
    CCrc32cTable() { Crc32c_GenerateTable(Values); }
};

UInt32 Z7_FASTCALL CrcUpdateT1_vhdx(UInt32 v, const void *data, size_t size, const UInt32 *table) {
    const Byte *p = (const Byte *)data;
    const Byte *pEnd = p + size;
    for (; p != pEnd; p++)
        v = table[(v ^ *p) & 0xff] ^ (v >> 8);
    return v;
}
} // namespace

// Function-local initialization is synchronized; subsequent access is read-only.
// Empty calls preserve raw state without forming a pointer from a null input.
extern "C" uint32_t
#ifdef _MSC_VER
    __cdecl
#endif
    sevenzip_crc32c_update(uint32_t state, const void *data, size_t size) {
    if (size == 0)
        return state;
    static const CCrc32cTable table;
    return CrcUpdateT1_vhdx(state, data, size, table.Values);
}
