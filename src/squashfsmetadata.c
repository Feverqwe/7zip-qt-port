// SPDX-License-Identifier: GPL-2.0-or-later
// Archive lifetime/cache services for the original Linux metadata readers.
#include "squashfsmetadata.h"
#include "squashfsio.h"
#include "fs/squashfs/squashfscompat.h"
#include "fs/squashfs/squashfs_fs.h"
#include "fs/squashfs/squashfs_fs_sb.h"
#include "fs/squashfs/squashfs_fs_i.h"
#include "fs/squashfs/squashfs.h"
#include "fs/squashfs/xattr.h"
#include "fs/squashfs/xattrcompat.h"
#include <limits.h>

struct sevenzip_squashfs_metadata {
    struct sevenzip_squashfs_io io;
    struct squashfs_sb_info msblk;
    struct super_block sb;
    struct squashfs_cache cache;
    struct squashfs_cache_entry entry;
    sevenzip_squashfs_read_meta read_meta;
    void *pages[1];
    unsigned char buffer[SQUASHFS_METADATA_SIZE];
    bool cached;
};

struct squashfs_cache_entry *
squashfs_cache_get(struct super_block *sb, struct squashfs_cache *cache, u64 block, int length) {
    struct sevenzip_squashfs_metadata *reader = cache->private_data;
    unsigned size = sizeof(reader->buffer);
    uint64_t next = block;
    (void)sb;
    (void)length;
    if (reader->cached && reader->entry.block == block)
        return &reader->entry;
    reader->entry.block = block;
    reader->entry.error = reader->read_meta(reader->io.opaque, block, reader->buffer, &size, &next);
    if (!reader->entry.error && (!size || size > sizeof(reader->buffer) || next <= block ||
                                 next > (uint64_t)reader->msblk.bytes_used))
        reader->entry.error = -EIO;
    reader->entry.length = (int)size;
    reader->entry.next_index = next;
    reader->cached = reader->entry.error == 0;
    return &reader->entry;
}
void squashfs_cache_put(struct squashfs_cache_entry *entry) { (void)entry; }

void sevenzip_squashfs_metadata_destroy(struct sevenzip_squashfs_metadata *reader) {
    if (!reader)
        return;
    free(reader->msblk.id_table);
    free(reader->msblk.fragment_index);
    free(reader->msblk.inode_lookup_table);
    free(reader->msblk.xattr_id_table);
    free(reader);
}

int sevenzip_squashfs_metadata_create(uint64_t bytes_used, sevenzip_squashfs_read read,
                                      sevenzip_squashfs_read_meta read_meta, void *opaque,
                                      struct sevenzip_squashfs_metadata **result,
                                      uint64_t *directory_end) {
    struct sevenzip_squashfs_metadata *reader;
    struct squashfs_sb_info *msblk;
    struct squashfs_super_block *disk;
    struct super_block *sb;
    u64 next_table, table_start;
    int error = -EINVAL;
    *result = NULL;
    if (!read || !read_meta || bytes_used < sizeof(*disk) || bytes_used > INT64_MAX)
        return -EINVAL;
    reader = calloc(1, sizeof(*reader));
    if (!reader)
        return -ENOMEM;
    reader->io.read = read;
    reader->io.opaque = opaque;
    reader->read_meta = read_meta;
    msblk = &reader->msblk;
    sb = &reader->sb;
    sb->s_fs_info = msblk;
    msblk->bytes_used = (long long)bytes_used;
    msblk->read_context = &reader->io;
    reader->cache.private_data = reader;
    msblk->block_cache = &reader->cache;
    reader->pages[0] = reader->buffer;
    reader->entry.data = reader->pages;
    disk = squashfs_read_table(sb, 0, sizeof(*disk));
    if (IS_ERR(disk)) {
        error = PTR_ERR(disk);
        goto failed;
    }
    msblk->inode_table = le64_to_cpu(disk->inode_table_start);
    msblk->directory_table = le64_to_cpu(disk->directory_table_start);
    msblk->inodes = le32_to_cpu(disk->inodes);
    msblk->fragments = le32_to_cpu(disk->fragments);
    msblk->ids = le16_to_cpu(disk->no_ids);

    // Table ordering and lookup sequence from squashfs_fill_super. The VFS
    // superblock/inode/cache allocation around this read path is not needed.
    table_start = le64_to_cpu(disk->xattr_id_table_start);
    next_table = bytes_used;
    if (table_start != SQUASHFS_INVALID_BLK) {
        msblk->xattr_id_table =
            squashfs_read_xattr_id_table(sb, table_start, &msblk->xattr_table, &msblk->xattr_ids);
        if (IS_ERR(msblk->xattr_id_table)) {
            error = PTR_ERR(msblk->xattr_id_table);
            msblk->xattr_id_table = NULL;
            goto failed_disk;
        }
        next_table = msblk->xattr_table;
    }
    msblk->id_table = squashfs_read_id_index_table(sb, le64_to_cpu(disk->id_table_start),
                                                   next_table, (unsigned short)msblk->ids);
    if (IS_ERR(msblk->id_table)) {
        error = PTR_ERR(msblk->id_table);
        msblk->id_table = NULL;
        goto failed_disk;
    }
    next_table = le64_to_cpu(msblk->id_table[0]);
    table_start = le64_to_cpu(disk->lookup_table_start);
    if (table_start != SQUASHFS_INVALID_BLK) {
        msblk->inode_lookup_table =
            squashfs_read_inode_lookup_table(sb, table_start, next_table, msblk->inodes);
        if (IS_ERR(msblk->inode_lookup_table)) {
            error = PTR_ERR(msblk->inode_lookup_table);
            msblk->inode_lookup_table = NULL;
            goto failed_disk;
        }
        next_table = le64_to_cpu(msblk->inode_lookup_table[0]);
    }
    if (msblk->fragments) {
        msblk->fragment_index = squashfs_read_fragment_index_table(
            sb, le64_to_cpu(disk->fragment_table_start), next_table, msblk->fragments);
        if (IS_ERR(msblk->fragment_index)) {
            error = PTR_ERR(msblk->fragment_index);
            msblk->fragment_index = NULL;
            goto failed_disk;
        }
        next_table = le64_to_cpu(msblk->fragment_index[0]);
    }
    if (msblk->directory_table > next_table || msblk->inode_table >= msblk->directory_table) {
        error = -EINVAL;
        goto failed_disk;
    }
    *directory_end = next_table;
    free(disk);
    *result = reader;
    return 0;
failed_disk:
    free(disk);
failed:
    sevenzip_squashfs_metadata_destroy(reader);
    return error;
}

int sevenzip_squashfs_id(struct sevenzip_squashfs_metadata *reader, unsigned index, unsigned *id) {
    return squashfs_get_id(&reader->sb, index, id);
}
int sevenzip_squashfs_inode(struct sevenzip_squashfs_metadata *reader, unsigned number,
                            uint64_t *reference) {
    long long result;
    if (!number || number > INT_MAX || !reader->msblk.inode_lookup_table)
        return -EINVAL;
    result = squashfs_inode_lookup(&reader->sb, (int)number);
    if (result < 0)
        return result >= -4095 ? (int)result : -EIO;
    *reference = (uint64_t)result;
    return 0;
}
int sevenzip_squashfs_fragment(struct sevenzip_squashfs_metadata *reader, unsigned index,
                               uint64_t *block, unsigned *size) {
    u64 start;
    int result = squashfs_frag_lookup(&reader->sb, index, &start);
    if (result < 0)
        return result;
    *block = start;
    *size = (unsigned)result;
    return 0;
}

static int xattr_inode(struct sevenzip_squashfs_metadata *reader, unsigned index,
                       struct inode *inode) {
    unsigned long long xattr;
    int result;
    memset(inode, 0, sizeof(*inode));
    inode->i_sb = &reader->sb;
    if (!reader->msblk.xattr_id_table || index == SQUASHFS_INVALID_XATTR)
        return 0;
    result = squashfs_xattr_lookup(&reader->sb, index, &inode->info.xattr_count,
                                   &inode->info.xattr_size, &xattr);
    if (result < 0)
        return result;
    if (inode->info.xattr_count < 0)
        return -EIO;
    inode->info.xattr = xattr;
    return 0;
}

int sevenzip_squashfs_xattrs(struct sevenzip_squashfs_metadata *reader, unsigned index,
                             char *buffer, size_t size) {
    struct inode inode;
    struct dentry dentry = {&inode};
    int result = xattr_inode(reader, index, &inode);
    if (result < 0)
        return result;
    return (int)squashfs_listxattr(&dentry, buffer, size);
}
int sevenzip_squashfs_xattr(struct sevenzip_squashfs_metadata *reader, unsigned index,
                            const char *name, void *buffer, size_t size) {
    struct inode inode;
    const struct xattr_handler *const *handler;
    int result = xattr_inode(reader, index, &inode);
    if (result < 0)
        return result;
    for (handler = squashfs_xattr_handlers; *handler; ++handler) {
        size_t prefix_size = strlen((*handler)->prefix);
        if (strncmp(name, (*handler)->prefix, prefix_size) == 0)
            return (*handler)->get(*handler, NULL, &inode, name + prefix_size, buffer, size);
    }
    return -EOPNOTSUPP;
}

int sevenzip_squashfs_validate_xattrs(struct sevenzip_squashfs_metadata *reader, unsigned index) {
    char *names, *name;
    int length, result = 0;
    if (!reader->msblk.xattr_id_table || index == SQUASHFS_INVALID_XATTR)
        return 0;
    length = sevenzip_squashfs_xattrs(reader, index, NULL, 0);
    if (length < 0)
        return length;
    if (length > 65536)
        return -EIO;
    names = malloc((size_t)length + 1);
    if (!names)
        return -ENOMEM;
    result = sevenzip_squashfs_xattrs(reader, index, names, (size_t)length);
    if (result != length) {
        free(names);
        return result < 0 ? result : -EIO;
    }
    names[length] = 0;
    for (name = names; name < names + length; name += strlen(name) + 1) {
        void *value;
        int size;
        if (strlen(name) > 255) {
            result = -EIO;
            break;
        }
        size = sevenzip_squashfs_xattr(reader, index, name, NULL, 0);
        if (size < 0 || size > 65536) {
            result = size < 0 ? size : -EIO;
            break;
        }
        value = malloc(size ? (size_t)size : 1);
        if (!value) {
            result = -ENOMEM;
            break;
        }
        result = sevenzip_squashfs_xattr(reader, index, name, value, (size_t)size);
        free(value);
        if (result < 0)
            break;
        result = 0;
    }
    free(names);
    return result;
}
