// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_MSDOS_READER_H
#define SEVENZIP_MSDOS_READER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Input/output positions use Linux block layer's 512-byte sector unit. */
struct sevenzip_msdos_parent {
    uint64_t offset, size;
    uint8_t type;
};
struct sevenzip_msdos_part {
    uint64_t offset, size;
    uint8_t parent_type;
};
typedef int32_t (*sevenzip_msdos_read)(void *, uint64_t, void *, unsigned);
int32_t sevenzip_msdos_scan(void *context, sevenzip_msdos_read read, uint64_t bytes,
                            const struct sevenzip_msdos_parent *parents, unsigned count,
                            unsigned logical_count, struct sevenzip_msdos_part *parts,
                            unsigned *part_count);
#ifdef __cplusplus
}
#endif
#endif
