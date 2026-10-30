/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SEVENZIP_MSDOS_USERSPACE_H
#define SEVENZIP_MSDOS_USERSPACE_H
#include "formats/mbr/msdosreader.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint8_t __u8;
typedef int16_t __s16;
typedef uint16_t __u16;
typedef uint16_t __le16;
typedef uint32_t __u32;
typedef uint32_t __le32;
typedef uint64_t sector_t;
#ifdef _MSC_VER
#pragma pack(push, 1)
#define __packed
#else
#define __packed __attribute__((__packed__))
#endif
#include "linux/include/linux/msdos_partition.h"
#ifdef _MSC_VER
#pragma pack(pop)
#endif
#undef __packed
#define CONFIG_SOLARIS_X86_PARTITION 1
#define CONFIG_BSD_DISKLABEL 1
#define CONFIG_UNIXWARE_DISKLABEL 1
#define CONFIG_MINIX_SUBPARTITION 1
struct parsed_partitions {
    void *context;
    sevenzip_msdos_read read;
    uint64_t bytes;
    int32_t error;
    struct sevenzip_msdos_part parts[256];
    int next, limit;
    uint8_t parent_type;
    char name[1];
    int pp_buf;
};
typedef struct {
    void *data;
} Sector;
static inline uint32_t get_unaligned_le32(const void *address) {
    const uint8_t *p = (const uint8_t *)address;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static inline uint32_t le32_to_cpu(uint32_t value) { return get_unaligned_le32(&value); }
static inline uint16_t le16_to_cpu(uint16_t value) {
    const uint8_t *p = (const uint8_t *)&value;
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}
void *read_part_sector(struct parsed_partitions *, sector_t, Sector *);
static inline void put_dev_sector(Sector sector) { free(sector.data); }
static inline void put_partition(struct parsed_partitions *state, int slot, sector_t offset,
                                 sector_t size) {
    if (slot < state->limit) {
        state->parts[slot].offset = offset;
        state->parts[slot].size = size;
        state->parts[slot].parent_type = state->parent_type;
    }
}
void sevenzip_msdos_log(void *, const char *, ...);
#define seq_buf_printf sevenzip_msdos_log
#define seq_buf_puts sevenzip_msdos_log
int sevenzip_msdos_subtypes(struct parsed_partitions *, const struct sevenzip_msdos_parent *,
                            unsigned);
#endif
