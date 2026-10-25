/* XzCrc64.c -- CRC64 calculation
2023-12-08 : Igor Pavlov : Public domain */

/* Preserve the SDK's uncomplemented streaming state around liblzma's API. */
#include "XzCrc64.h"
#include "Precomp.h"

#define LZMA_API_STATIC
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4668) // Unchanged liblzma API checks optional feature macros.
#endif
#include "xz/src/liblzma/api/lzma.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

UInt64 Z7_FASTCALL Crc64Update(UInt64 crc, const void *data, size_t size) {
    if (!size)
        return crc;
    return ~lzma_crc64((const uint8_t *)data, size, ~crc);
}

void Z7_FASTCALL Crc64GenerateTable(void) {
    /* liblzma's generic implementation has an immutable, precomputed table. */
}
