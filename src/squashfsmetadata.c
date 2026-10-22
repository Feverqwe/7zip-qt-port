// SPDX-License-Identifier: GPL-2.0-or-later
// Archive lifetime/cache services for the original Linux metadata readers.
#include "squashfsmetadata.h"
#include "fs/squashfs/squashfs.h"
#include "fs/squashfs/squashfs_fs.h"
#include "fs/squashfs/squashfs_fs_i.h"
#include "fs/squashfs/squashfs_fs_sb.h"
#include "fs/squashfs/squashfscompat.h"
#include "fs/squashfs/xattr.h"
#include "fs/squashfs/xattrcompat.h"
#include "squashfsio.h"
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
    unsigned unavailable;
    bool inode_only;
};

struct squashfs_cache_entry *
squashfs_cache_get(struct super_block *sb, struct squashfs_cache *cache, u64 block, int length) {
    struct sevenzip_squashfs_metadata *reader = cache->private_data;
    unsigned size = sizeof(reader->buffer);
    uint64_t next = block;
    (void)sb;
    (void)length;
    if (reader->inode_only && (block < reader->msblk.inode_table ||
                              block >= reader->msblk.directory_table)) {
        reader->entry.error = -EIO;
        reader->cached = false;
        return &reader->entry;
    }
    if (reader->cached && reader->entry.block == block)
        return &reader->entry;
    reader->entry.block = block;
    reader->entry.error = reader->read_meta(reader->io.opaque, block, reader->buffer, &size, &next);
    if (!reader->entry.error && (!size || size > sizeof(reader->buffer) || next <= block ||
                                 next > (uint64_t)reader->msblk.bytes_used ||
                                 (reader->inode_only && next > reader->msblk.directory_table)))
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
    free(reader->msblk.meta_index);
    free(reader);
}

struct sevenzip_squashfs_file {
    struct sevenzip_squashfs_metadata reader;
    struct inode inode;
    uint64_t blocks;
    uint64_t next_index, next_block, next_data;
    int next_offset;
    bool cursor_valid;
};

void sevenzip_squashfs_file_destroy(struct sevenzip_squashfs_file *file) {
    if (file) {
        free(file->reader.msblk.meta_index);
        free(file);
    }
}

int sevenzip_squashfs_file_create(uint64_t bytes_used, uint64_t inode_table,
    uint64_t directory_table, unsigned block_size, uint64_t reference,
    unsigned header_size, uint64_t file_size, uint64_t block_count, uint64_t data_start,
    unsigned inode_number, sevenzip_squashfs_read_meta read_meta, void *opaque,
    struct sevenzip_squashfs_file **result) {
    struct sevenzip_squashfs_file *file;
    struct sevenzip_squashfs_metadata *reader;
    unsigned log = 0;
    int error;
    *result = NULL;
    if (!read_meta || bytes_used > INT64_MAX || inode_table >= directory_table ||
        directory_table > bytes_used || data_start > bytes_used || !inode_number ||
        (header_size != 32 && header_size != 56) || block_size < 4096 ||
        block_size > SQUASHFS_FILE_MAX_SIZE || (block_size & (block_size - 1)) ||
        (reference >> 16) >= directory_table - inode_table)
        return -EINVAL;
    file = calloc(1, sizeof(*file));
    if (!file)
        return -ENOMEM;
    reader = &file->reader;
    reader->io.opaque = opaque;
    reader->read_meta = read_meta;
    reader->inode_only = true;
    reader->msblk.inode_table = inode_table;
    reader->msblk.directory_table = directory_table;
    reader->msblk.bytes_used = (long long)bytes_used;
    reader->msblk.block_size = block_size;
    while ((1U << log) < block_size)
        ++log;
    reader->msblk.block_log = log;
    if (block_count < (file_size >> log) ||
        block_count > (file_size >> log) + ((file_size & (block_size - 1)) != 0)) {
        sevenzip_squashfs_file_destroy(file);
        return -EINVAL;
    }
    reader->sb.s_fs_info = &reader->msblk;
    reader->cache.private_data = reader;
    reader->msblk.block_cache = &reader->cache;
    reader->pages[0] = reader->buffer;
    reader->entry.data = reader->pages;
    file->inode.i_sb = &reader->sb;
    file->inode.i_size = file_size;
    file->inode.i_ino = inode_number;
    file->blocks = block_count;
    file->inode.info.start = data_start;
    file->inode.info.block_list_start = inode_table + (reference >> 16);
    file->inode.info.offset = (int)(reference & 0xffff);
    error = squashfs_read_metadata(&reader->sb, NULL, &file->inode.info.block_list_start,
                                   &file->inode.info.offset, (int)header_size);
    if (error < 0) {
        sevenzip_squashfs_file_destroy(file);
        return error;
    }
    file->next_index = 0;
    file->next_block = file->inode.info.block_list_start;
    file->next_offset = file->inode.info.offset;
    file->next_data = data_start;
    file->cursor_valid = true;
    *result = file;
    return 0;
}

int sevenzip_squashfs_file_block(struct sevenzip_squashfs_file *file, uint64_t index,
    uint64_t *start, unsigned *size) {
    int result;
    if (!file || index > INT_MAX || index >= file->blocks)
        return -EINVAL;
    if (file->cursor_valid && index == file->next_index) {
        *start = file->next_data;
        result = squashfs_read_blocklist_next(&file->inode, &file->next_block, &file->next_offset);
    } else {
        *start = 0;
        result = squashfs_read_blocklist_ptrs(&file->inode, (int)index,
            &file->next_block, &file->next_offset, start);
    }
    file->cursor_valid = false;
    if (result < 0)
        return result;
    *size = SQUASHFS_COMPRESSED_SIZE_BLOCK((unsigned)result);
    if (*size > file->reader.msblk.block_size || (*size == 0 && result != 0) ||
        *start > (uint64_t)file->reader.msblk.bytes_used ||
        *size > (uint64_t)file->reader.msblk.bytes_used - *start)
        return -EIO;
    *size = (unsigned)result;
    file->next_index = index + 1;
    file->next_data = *start + SQUASHFS_COMPRESSED_SIZE_BLOCK(*size);
    file->cursor_valid = true;
    return 0;
}

unsigned sevenzip_squashfs_unavailable_tables(const struct sevenzip_squashfs_metadata *reader) {
    return reader->unavailable;
}

static bool fatal_table_error(int error) {
    return error == -ENOMEM || error == -ECANCELED || error == -95 || error == -EOPNOTSUPP;
}

// A failed neighbouring table cannot establish an index's exact end. Its own
// declared count still bounds the original Linux loader within bytes_used.
static u64 index_end(struct squashfs_sb_info *msblk, u64 start, u64 length) {
    const u64 end = (u64)msblk->bytes_used;
    return start <= end && length <= end - start ? start + length : 0;
}

static bool metadata_boundary(struct squashfs_sb_info *msblk, u64 start, u64 *end) {
    if (start < msblk->directory_table || start > (u64)msblk->bytes_used)
        return false;
    if (start < *end)
        *end = start;
    return true;
}

int sevenzip_squashfs_metadata_create(uint64_t bytes_used, sevenzip_squashfs_read read,
                                      sevenzip_squashfs_read_meta read_meta, void *opaque,
                                      struct sevenzip_squashfs_metadata **result,
                                      uint64_t *directory_end) {
    struct sevenzip_squashfs_metadata *reader;
    struct squashfs_sb_info *msblk;
    struct squashfs_super_block *disk;
    struct super_block *sb;
    u64 next_table, table_start, directory_limit;
    bool next_known = true;
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
    if (msblk->inode_table >= msblk->directory_table || msblk->directory_table > bytes_used) {
        error = -EINVAL;
        goto failed_disk;
    }
    directory_limit = bytes_used;

    // Keep fill_super's loaders/order, but isolate ancillary failures for the
    // read-only archive view. Missing indexes never become fabricated entries.
    table_start = le64_to_cpu(disk->xattr_id_table_start);
    next_table = bytes_used;
    if (table_start != SQUASHFS_INVALID_BLK) {
        msblk->xattr_id_table =
            squashfs_read_xattr_id_table(sb, table_start, &msblk->xattr_table, &msblk->xattr_ids);
        if (IS_ERR(msblk->xattr_id_table)) {
            error = PTR_ERR(msblk->xattr_id_table);
            msblk->xattr_id_table = NULL;
            if (fatal_table_error(error))
                goto failed_disk;
        }
        next_known =
            msblk->xattr_id_table && metadata_boundary(msblk, msblk->xattr_table, &directory_limit);
        if (next_known)
            next_table = msblk->xattr_table;
        else {
            free(msblk->xattr_id_table);
            msblk->xattr_id_table = NULL;
            reader->unavailable |= SEVENZIP_SQUASHFS_XATTR_UNAVAILABLE;
        }
    }
    table_start = le64_to_cpu(disk->id_table_start);
    msblk->id_table = squashfs_read_id_index_table(
        sb, table_start,
        next_known ? next_table
                   : index_end(msblk, table_start, SQUASHFS_ID_BLOCK_BYTES(msblk->ids)),
        (unsigned short)msblk->ids);
    if (IS_ERR(msblk->id_table)) {
        error = PTR_ERR(msblk->id_table);
        msblk->id_table = NULL;
        if (fatal_table_error(error))
            goto failed_disk;
    }
    next_known = msblk->id_table &&
                 metadata_boundary(msblk, le64_to_cpu(msblk->id_table[0]), &directory_limit);
    if (next_known)
        next_table = le64_to_cpu(msblk->id_table[0]);
    else {
        free(msblk->id_table);
        msblk->id_table = NULL;
        reader->unavailable |= SEVENZIP_SQUASHFS_IDS_UNAVAILABLE;
    }
    table_start = le64_to_cpu(disk->lookup_table_start);
    if (table_start != SQUASHFS_INVALID_BLK) {
        msblk->inode_lookup_table = squashfs_read_inode_lookup_table(
            sb, table_start,
            next_known ? next_table
                       : index_end(msblk, table_start, SQUASHFS_LOOKUP_BLOCK_BYTES(msblk->inodes)),
            msblk->inodes);
        if (IS_ERR(msblk->inode_lookup_table)) {
            error = PTR_ERR(msblk->inode_lookup_table);
            msblk->inode_lookup_table = NULL;
            if (fatal_table_error(error))
                goto failed_disk;
        }
        next_known =
            msblk->inode_lookup_table &&
            metadata_boundary(msblk, le64_to_cpu(msblk->inode_lookup_table[0]), &directory_limit);
        if (next_known)
            next_table = le64_to_cpu(msblk->inode_lookup_table[0]);
        else {
            free(msblk->inode_lookup_table);
            msblk->inode_lookup_table = NULL;
            reader->unavailable |= SEVENZIP_SQUASHFS_EXPORT_UNAVAILABLE;
        }
    }
    if (msblk->fragments) {
        table_start = le64_to_cpu(disk->fragment_table_start);
        msblk->fragment_index = squashfs_read_fragment_index_table(
            sb, table_start,
            next_known
                ? next_table
                : index_end(msblk, table_start, SQUASHFS_FRAGMENT_INDEX_BYTES(msblk->fragments)),
            msblk->fragments);
        if (IS_ERR(msblk->fragment_index)) {
            error = PTR_ERR(msblk->fragment_index);
            msblk->fragment_index = NULL;
            if (fatal_table_error(error))
                goto failed_disk;
        }
        if (msblk->fragment_index &&
            metadata_boundary(msblk, le64_to_cpu(msblk->fragment_index[0]), &directory_limit))
            next_table = le64_to_cpu(msblk->fragment_index[0]);
        else {
            free(msblk->fragment_index);
            msblk->fragment_index = NULL;
            reader->unavailable |= SEVENZIP_SQUASHFS_FRAGMENTS_UNAVAILABLE;
        }
    }
    *directory_end = directory_limit;
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
    if (!reader->msblk.id_table)
        return -EIO;
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
    if (!reader->msblk.fragment_index)
        return -EIO;
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
    if (index == SQUASHFS_INVALID_XATTR)
        return 0;
    if (reader->unavailable & SEVENZIP_SQUASHFS_XATTR_UNAVAILABLE)
        return -EIO;
    if (!reader->msblk.xattr_id_table)
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

int sevenzip_squashfs_visit_xattrs(struct sevenzip_squashfs_metadata *reader, unsigned index,
                                   sevenzip_squashfs_xattr_emit emit, void *context) {
    char *names, *name;
    int length, result = 0, incomplete = 0;
    if (index == SQUASHFS_INVALID_XATTR)
        return 0;
    if (reader->unavailable & SEVENZIP_SQUASHFS_XATTR_UNAVAILABLE)
        return -EIO;
    if (!reader->msblk.xattr_id_table)
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
            if (result == -ENOMEM || result == -ECANCELED)
                break;
            incomplete = result;
            result = 0;
            continue;
        }
        value = malloc(size ? (size_t)size : 1);
        if (!value) {
            result = -ENOMEM;
            break;
        }
        result = sevenzip_squashfs_xattr(reader, index, name, value, (size_t)size);
        if (result == size) {
            result = emit ? emit(context, name, value, (size_t)size) : 0;
            free(value);
            if (result)
                break;
            continue;
        }
        if (result >= 0)
            result = -EIO;
        free(value);
        if (result == -ENOMEM || result == -ECANCELED)
            break;
        incomplete = result;
        result = 0;
    }
    free(names);
    return result ? result : incomplete;
}
int sevenzip_squashfs_validate_xattrs(struct sevenzip_squashfs_metadata *reader, unsigned index) {
    return sevenzip_squashfs_visit_xattrs(reader, index, NULL, NULL);
}
