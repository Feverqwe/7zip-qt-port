// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/hfsplus/xattr.c
 * Vyacheslav Dubeyko <slava@dubeyko.com>
 * hfsplus_getxattr_finder_info: catalog reads are supplied by the existing
 * archive reader; user_info/finder_info bytes and size semantics are retained.
 */
#include "hfscompat.h"

/* __hfsplus_getxattr: the archive reader has located a bounded attribute-tree
 * record. Replace inode/B-tree lookup and the temporary slab entry with that
 * record; retain the Linux record-type, 16-bit length and caller-size rules. */
int __hfsplus_getxattr(const void *record, size_t length, void *value, size_t size)
{
    const u8 *entry = record;
    __be32 xattr_record_type;
    u32 record_type;
    __be16 disk_length;
    u16 record_length = 0;
    const size_t header = offsetof(struct hfsplus_attr_inline_data, raw_bytes);

    if (length < sizeof(xattr_record_type))
        return -EIO;
    memcpy(&xattr_record_type, entry, sizeof(xattr_record_type));
    record_type = be32_to_cpu(xattr_record_type);
    if (record_type == HFSPLUS_ATTR_INLINE_DATA) {
        if (length < header)
            return -EIO;
        memcpy(&disk_length, entry + offsetof(struct hfsplus_attr_inline_data, length),
               sizeof(disk_length));
        record_length = be16_to_cpu(disk_length);
        if (record_length > HFSPLUS_MAX_INLINE_DATA_SIZE)
            return -EIO;
        if (record_length > length - header)
            return -EIO;
    } else if (record_type == HFSPLUS_ATTR_FORK_DATA ||
               record_type == HFSPLUS_ATTR_EXTENTS) {
        return -EOPNOTSUPP;
    } else {
        return -EIO;
    }

    if (size >= record_length) {
        if (record_length)
            memcpy(value, entry + header, record_length);
        return record_length;
    }
    return size ? -ERANGE : record_length;
}

int hfsplus_getxattr_finder_info(const void *record, size_t length, void *value, size_t size)
{
    const u8 *entry = record;
    u16 entry_type;
    if (length < 2)
        return -EIO;
    entry_type = (u16)((u16)entry[0] << 8 | entry[1]);
    if (entry_type != 1 && entry_type != 2)
        return -EOPNOTSUPP;
    if (hfsplus_brec_read_cat(record, length))
        return -EIO;
    if (size >= 32) {
        memcpy(value, entry + 48, 32);
        return 32;
    }
    return size ? -ERANGE : 32;
}
