// Shared raw CRC32C state API; extracted VHDX implementation is LGPL-2.1-or-later.
#ifndef SEVENZIP_VHDX_CRC_H
#define SEVENZIP_VHDX_CRC_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#ifdef _MSC_VER
uint32_t __cdecl sevenzip_crc32c_update(uint32_t state, const void *data, size_t size);
#else
uint32_t sevenzip_crc32c_update(uint32_t state, const void *data, size_t size);
#endif
#ifdef __cplusplus
}
#endif
#endif
