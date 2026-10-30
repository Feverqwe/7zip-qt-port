// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_UDF_TYPES_H
#define SEVENZIP_UDF_TYPES_H
#include <stdint.h>
typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint64_t __le64;
#ifdef _MSC_VER
#define __packed
#else
#define __packed __attribute__((packed))
#endif
#endif
