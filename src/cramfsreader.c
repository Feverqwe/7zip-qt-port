// SPDX-License-Identifier: GPL-2.0-only
// Userspace memory/endian/lifetime services for the Linux cramfs reader.
#include "fs/cramfs/internal.h"

const void *cramfs_read(struct super_block *sb, u32 offset, u32 length) {
    if (offset > sb->source->size || length > sb->source->size - offset) {
        sb->error = true;
        return sb->zero;
    }
    if (!sb->source->data) {
        if (length > sizeof(sb->zero) ||
            sb->source->read(sb->source->context, offset, sb->zero, length)) {
            sb->error = true;
            memset(sb->zero, 0, sizeof(sb->zero));
        }
        return sb->zero;
    }
    return sb->source->data + offset;
}

u32 cramfs_read_u32(struct super_block *sb, u32 offset) {
    const unsigned char *p = cramfs_read(sb, offset, 4);
    if (sb->source->big_endian)
        return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
    return p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

uint16_t cramfs_read_u16(struct super_block *sb, u32 offset) {
    const unsigned char *p = cramfs_read(sb, offset, 2);
    return sb->source->big_endian ? (uint16_t)((p[0] << 8) | p[1])
                                 : (uint16_t)(p[0] | (p[1] << 8));
}

int cramfs_read_block(const struct cramfs_source *source, uint32_t table, uint32_t size,
                     uint32_t block, unsigned char page[4096]) {
    struct super_block sb = {0};
    sb.source = source;
    struct inode inode = {&sb, size, table};
    struct address_space mapping = {&inode};
    struct folio folio = {&mapping, block, page, false};
    if (table > source->size || ((uint64_t)size + PAGE_SIZE - 1) / PAGE_SIZE * 4 > source->size - table)
        return -1;
    cramfs_read_folio(NULL, &folio);
    return !folio.success || sb.error ? -1 : 0;
}
