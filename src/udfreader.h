// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_UDF_READER_H
#define SEVENZIP_UDF_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*sevenzip_udf_read)(void *, uint16_t, uint32_t, void *, size_t);
typedef int (*sevenzip_udf_extent)(void *, unsigned, uint32_t, uint32_t, uint16_t);
int sevenzip_udf_extents(void *, sevenzip_udf_read, sevenzip_udf_extent,
                         uint32_t, uint32_t, uint16_t, unsigned, int,
                         const void *, uint32_t, uint64_t, uint32_t);
#ifdef __cplusplus
}
#endif
#endif
