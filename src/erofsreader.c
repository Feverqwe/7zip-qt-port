// SPDX-License-Identifier: GPL-2.0-only
// Stream, metadata-buffer and lifetime services for the Linux EROFS reader.
#include "erofsreader.h"
#include "fs/erofs/internal.h"

struct erofs_volume {
    struct super_block sb;
    struct erofs_sb_info info;
};
struct erofs_file {
    struct erofs_inode inode;
    struct erofs_map_blocks map;
    u8 *decoded;
    u64 decoded_start, decoded_size;
};

void *kmemdup_nul(const void *data, size_t size, int flags) {
    (void)flags;
    if (size == SIZE_MAX)
        return NULL;
    char *copy = malloc(size + 1);
    if (copy) {
        memcpy(copy, data, size);
        copy[size] = 0;
    }
    return copy;
}
char *kstrndup(const void *data, size_t size, int flags) {
    const char *end = memchr(data, 0, size);
    return kmemdup_nul(data, end ? (size_t)(end - (const char *)data) : size, flags);
}
void super_set_uuid(struct super_block *sb, const void *uuid, size_t size) {
    (void)sb; (void)uuid; (void)size;
}
void erofs_put_metabuf(struct erofs_buf *buf) {
    free(buf->base);
    memset(buf, 0, sizeof(*buf));
}
void erofs_unmap_metabuf(struct erofs_buf *buf) {
    // An owned userspace buffer stays mapped until its owner releases it.
    (void)buf;
}
int erofs_init_metabuf(struct erofs_buf *buf, struct super_block *sb, bool in_metabox) {
    if (buf->sb != sb || buf->in_metabox != in_metabox)
        erofs_put_metabuf(buf);
    buf->sb = sb;
    buf->in_metabox = in_metabox;
    return in_metabox && !EROFS_SB(sb)->metabox_inode ? -EFSCORRUPTED : 0;
}
void *erofs_bread(struct erofs_buf *buf, erofs_off_t offset, bool need_kmap) {
    struct super_block *sb = buf->sb;
    if (!sb)
        return ERR_PTR(-EINVAL);
    if (sb->poll && sb->poll(sb->context))
        return ERR_PTR(-ECANCELED);
    struct inode *meta = EROFS_SB(sb)->metabox_inode;
    u64 length = buf->in_metabox ? (u64)meta->i_size : sb->image_size;
    if (offset >= length)
        return ERR_PTR(-EIO);
    u64 index = offset >> PAGE_SHIFT;
    if (!buf->base || index != buf->index) {
        if (!buf->base) {
            buf->base = malloc(PAGE_SIZE);
            if (!buf->base)
                return ERR_PTR(-ENOMEM);
        }
        buf->index = UINT64_MAX;
        u64 pos = index << PAGE_SHIFT;
        size_t size = (size_t)min((u64)PAGE_SIZE, length - pos);
        int error;
        if (buf->in_metabox)
            error = sb->read_inode(meta, pos, buf->base, size);
        else
            error = sb->read(sb->context, 0, pos, buf->base, size) ? -EIO : 0;
        if (error)
            return ERR_PTR(error);
        memset(buf->base + size, 0, PAGE_SIZE - size);
        buf->index = index;
    }
    return need_kmap ? buf->base + (offset & (PAGE_SIZE - 1)) : NULL;
}
void *erofs_read_metabuf(struct erofs_buf *buf, struct super_block *sb,
                         erofs_off_t offset, bool in_metabox) {
    int error = erofs_init_metabuf(buf, sb, in_metabox);
    return error ? ERR_PTR(error) : erofs_bread(buf, offset, true);
}

static int erofs_read_physical(struct inode *inode, struct erofs_map_blocks *map, void *data) {
    struct super_block *sb = inode->i_sb;
    struct erofs_sb_info *sbi = EROFS_SB(sb);
    if ((map->m_flags & EROFS_MAP_META) && erofs_inode_in_metabox(inode))
        return sb->read_inode(sbi->metabox_inode, map->m_pa, data, (size_t)map->m_plen);
    u64 pos = map->m_pa;
    unsigned int device = map->m_deviceid;
    if (device) {
        if (device > sbi->extra_devices)
            return -ENODEV;
        if (sbi->flatdev) {
            const u64 start = erofs_pos(sb, sbi->devices[device - 1].uniaddr);
            if (check_add_overflow(pos, start, &pos))
                return -EFSCORRUPTED;
            device = 0;
        }
    } else if (!sbi->flatdev) {
        for (unsigned int i = 0; i < sbi->extra_devices; ++i) {
            const struct erofs_device_info *dif = &sbi->devices[i];
            const u64 start = erofs_pos(sb, dif->uniaddr);
            if (start && pos >= start && erofs_blknr(sb, pos - start) < dif->blocks) {
                pos -= start;
                device = i + 1;
                break;
            }
        }
    }
    u64 length = device ? erofs_pos(sb, sbi->devices[device - 1].blocks) : sb->image_size;
    if (pos > length || map->m_plen > length - pos)
        return -EIO;
    return sb->read(sb->context, device, pos, data, (size_t)map->m_plen) ? -EIO : 0;
}

void erofs_close_file(struct erofs_file *file) {
    if (!file)
        return;
    free(file->inode.vfs_inode.i_link);
    erofs_put_metabuf(&file->map.buf);
    free(file->decoded);
    free(file);
}
int erofs_open_file(struct erofs_volume *volume, uint64_t nid, struct erofs_file **result) {
    *result = NULL;
    const u64 lo = nid & EROFS_DIRENT_NID_MASK;
    if (lo > (UINT64_MAX - erofs_pos(&volume->sb, volume->info.meta_blkaddr)) >> 5)
        return -EFSCORRUPTED;
    struct erofs_file *file = calloc(1, sizeof(*file));
    if (!file)
        return -ENOMEM;
    file->inode.nid = nid;
    file->inode.vfs_inode.i_sb = &volume->sb;
    file->inode.vfs_inode.i_blkbits = volume->sb.s_blocksize_bits;
    int error = erofs_read_inode(&file->inode.vfs_inode);
    if (!error && file->inode.datalayout != EROFS_INODE_CHUNK_BASED &&
        !erofs_inode_is_data_compressed(file->inode.datalayout) &&
        file->inode.startblk != (u64)EROFS_NULL_ADDR) {
        // A VFS block device bounds these addresses in the kernel.  Keep the
        // flat-extent addition from wrapping before the stream boundary checks.
        u64 end;
        u64 length = round_up((u64)file->inode.vfs_inode.i_size, volume->sb.s_blocksize);
        if (file->inode.datalayout == EROFS_INODE_FLAT_INLINE && length)
            length -= volume->sb.s_blocksize;
        if (file->inode.startblk > (UINT64_MAX >> volume->sb.s_blocksize_bits) ||
            check_add_overflow(erofs_pos(&volume->sb, file->inode.startblk), length, &end))
            error = -EFSCORRUPTED;
    }
    if (error) {
        erofs_close_file(file);
        return error;
    }
    *result = file;
    return 0;
}
void erofs_get_file_info(const struct erofs_file *file, struct erofs_item *item) {
    const struct inode *inode = &file->inode.vfs_inode;
    *item = (struct erofs_item) {
        .nid = file->inode.nid, .size = (u64)inode->i_size,
        .packed_size = inode->i_blocks << 9, .mtime = inode->mtime,
        .nsec = inode->nsec, .mode = inode->i_mode, .uid = inode->uid, .gid = inode->gid,
        .nlink = inode->nlink, .rdev = inode->i_rdev, .layout = file->inode.datalayout
    };
}
int erofs_pread_file(struct erofs_file *file, uint64_t offset, void *data, size_t size,
                      size_t *processed) {
    struct inode *inode = &file->inode.vfs_inode;
    struct super_block *sb = inode->i_sb;
    struct erofs_sb_info *sbi = EROFS_SB(sb);
    u8 *dest = data;
    *processed = 0;
    if (offset >= (u64)inode->i_size)
        return 0;
    size = (size_t)min((u64)size, (u64)inode->i_size - offset);
    while (*processed < size) {
        if (sb->poll && sb->poll(sb->context))
            return -ECANCELED;
        size_t count = size - *processed;
        if (file->decoded && offset >= file->decoded_start &&
            offset - file->decoded_start < file->decoded_size) {
            count = (size_t)min((u64)count, file->decoded_size - (offset - file->decoded_start));
            memcpy(dest, file->decoded + (offset - file->decoded_start), count);
        } else {
            struct erofs_map_blocks *map = &file->map;
            map->m_la = offset;
            map->m_llen = count;
            int error = erofs_inode_is_data_compressed(file->inode.datalayout)
                ? z_erofs_map_blocks_iter(inode, map, EROFS_GET_BLOCKS_FIEMAP)
                : erofs_map_blocks(inode, map);
            if (error)
                return error;
            if (map->m_la > offset || !map->m_llen || offset - map->m_la >= map->m_llen)
                return -EFSCORRUPTED;
            u64 within = offset - map->m_la;
            count = (size_t)min((u64)count, map->m_llen - within);
            if (map->m_flags & EROFS_MAP_FRAGMENT) {
                u64 pos;
                if (!sbi->packed_inode ||
                    check_add_overflow(file->inode.z_fragmentoff, within, &pos))
                    return -EFSCORRUPTED;
                error = sb->read_inode(sbi->packed_inode, pos, dest, count);
            } else if (!(map->m_flags & EROFS_MAP_MAPPED)) {
                memset(dest, 0, count);
            } else if (!erofs_inode_is_data_compressed(file->inode.datalayout)) {
                if (check_add_overflow(map->m_pa, within, &map->m_pa))
                    return -EFSCORRUPTED;
                map->m_plen = count;
                error = erofs_read_physical(inode, map, dest);
            } else {
                free(file->decoded);
                file->decoded = NULL;
                file->decoded_size = 0;
                u8 *input = malloc((size_t)map->m_plen);
                u8 *output = malloc((size_t)map->m_llen);
                if (!input || !output) {
                    free(input); free(output);
                    return -ENOMEM;
                }
                error = erofs_read_physical(inode, map, input);
                if (!error)
                    error = erofs_decode_extent(inode, map, input, output);
                free(input);
                if (error) {
                    free(output);
                    return error;
                }
                file->decoded = output;
                file->decoded_start = map->m_la;
                file->decoded_size = map->m_llen;
                memcpy(dest, output + within, count);
            }
            if (error)
                return error;
        }
        *processed += count;
        dest += count;
        offset += count;
    }
    return 0;
}

static int erofs_pread_inode(struct inode *inode, u64 pos, void *data, size_t size) {
    struct super_block *sb = inode->i_sb;
    if (sb->read_depth >= 16)
        return -EFSCORRUPTED;
    ++sb->read_depth;
    struct erofs_file file = {0};
    file.inode = *EROFS_I(inode);
    size_t processed;
    int error = erofs_pread_file(&file, pos, data, size, &processed);
    *EROFS_I(inode) = file.inode;
    erofs_put_metabuf(&file.map.buf);
    free(file.decoded);
    --sb->read_depth;
    return error ? error : processed == size ? 0 : -EIO;
}

void erofs_close_volume(struct erofs_volume *volume) {
    if (!volume)
        return;
    if (volume->info.metabox_inode) {
        free(volume->info.metabox_inode->i_link);
        free(EROFS_I(volume->info.metabox_inode));
    }
    if (volume->info.packed_inode) {
        free(volume->info.packed_inode->i_link);
        free(EROFS_I(volume->info.packed_inode));
    }
    free(volume->info.devices);
    free(volume->info.volume_name);
    free(volume);
}
static int erofs_internal_inode(struct erofs_volume *v, u64 nid, struct inode **result) {
    struct erofs_file *file;
    int error = erofs_open_file(v, nid, &file);
    if (error)
        return error;
    struct erofs_inode *inode = malloc(sizeof(*inode));
    if (!inode) {
        erofs_close_file(file);
        return -ENOMEM;
    }
    *inode = file->inode;
    file->inode.vfs_inode.i_link = NULL;
    erofs_close_file(file);
    *result = &inode->vfs_inode;
    return 0;
}
int erofs_open_volume(const struct erofs_source *source, struct erofs_volume **result) {
    *result = NULL;
    if (!source || !source->read || source->size > INT64_MAX)
        return -EINVAL;
    struct erofs_volume *v = calloc(1, sizeof(*v));
    if (!v)
        return -ENOMEM;
    v->sb.s_fs_info = &v->info;
    v->sb.s_blocksize = PAGE_SIZE;
    v->sb.s_blocksize_bits = PAGE_SHIFT;
    v->sb.image_size = source->size;
    v->sb.context = source->context;
    v->sb.read = source->read;
    v->sb.poll = source->poll;
    v->sb.decode = source->decode;
    v->sb.open_device = source->open_device;
    v->sb.read_inode = erofs_pread_inode;
    v->info.flatdev = !source->external_devices;
    int error = erofs_read_superblock(&v->sb);
    if (!error) {
        v->sb.s_blocksize_bits = v->info.blkszbits;
        v->sb.s_blocksize = 1U << v->info.blkszbits;
        u64 blocks = v->info.dif0.blocks;
        if (v->info.flatdev) {
            for (unsigned int i = 0; i < v->info.extra_devices; ++i) {
                u64 end;
                if (check_add_overflow(v->info.devices[i].uniaddr, v->info.devices[i].blocks, &end)) {
                    error = -EFSCORRUPTED;
                    break;
                }
                blocks = max(blocks, end);
            }
        }
        if (blocks > (UINT64_MAX >> v->info.blkszbits) ||
            erofs_pos(&v->sb, blocks) > source->size ||
            erofs_pos(&v->sb, v->info.dif0.blocks) < EROFS_SUPER_OFFSET + v->info.sb_size)
            error = -EIO;
        if (!error)
            v->sb.image_size = erofs_pos(&v->sb, blocks);
    }
    if (!error && erofs_sb_has_fragments(&v->info) && v->info.packed_nid)
        error = erofs_internal_inode(v, v->info.packed_nid, &v->info.packed_inode);
    if (!error && erofs_sb_has_metabox(&v->info))
        error = erofs_internal_inode(v, v->info.metabox_nid, &v->info.metabox_inode);
    if (!error) {
        struct erofs_file *root;
        error = erofs_open_file(v, v->info.root_nid, &root);
        if (!error) {
            if (!S_ISDIR(root->inode.vfs_inode.i_mode))
                error = -EFSCORRUPTED;
            erofs_close_file(root);
        }
    }
    if (error) {
        erofs_close_volume(v);
        return error;
    }
    *result = v;
    return 0;
}
void erofs_get_volume_info(const struct erofs_volume *v, struct erofs_volume_info *info) {
    *info = (struct erofs_volume_info) {
        .size = v->sb.image_size, .root_nid = v->info.root_nid,
        .inode_count = v->info.inos, .block_size = v->sb.s_blocksize,
        .compat = v->info.feature_compat, .incompat = v->info.feature_incompat,
        .extra_devices = v->info.extra_devices
    };
    if (v->info.volume_name)
        memcpy(info->name, v->info.volume_name, strlen(v->info.volume_name));
}
int erofs_device_tag(const struct erofs_volume *v, unsigned int id, char tag[65]) {
    if (!id || id > v->info.extra_devices)
        return -EINVAL;
    memcpy(tag, v->info.devices[id - 1].tag, 64);
    tag[64] = 0;
    return 0;
}

struct erofs_directory_emitter {
    void *context;
    int (*emit)(void *, const char *, size_t, u64, unsigned int);
    int result;
};
static bool erofs_emit(void *opaque, const char *name, unsigned int size, u64 nid, u8 type) {
    struct erofs_directory_emitter *emitter = opaque;
    emitter->result = emitter->emit(emitter->context, name, size, nid, type);
    return emitter->result == 0;
}
int erofs_read_directory(struct erofs_file *file, void *context,
                         int (*emit)(void *, const char *, size_t, u64, unsigned int)) {
    struct inode *inode = &file->inode.vfs_inode;
    if (!S_ISDIR(inode->i_mode))
        return -EINVAL;
    const unsigned int blocksize = i_blocksize(inode);
    u8 *block = malloc(blocksize);
    if (!block)
        return -ENOMEM;
    struct erofs_directory_emitter emitter = {context, emit, 0};
    struct dir_context ctx = {0, &emitter, erofs_emit};
    int error = 0;
    for (u64 pos = 0; pos < (u64)inode->i_size; pos += blocksize) {
        size_t size;
        error = erofs_pread_file(file, pos, block, blocksize, &size);
        if (error)
            break;
        if (size < sizeof(struct erofs_dirent)) {
            error = -EFSCORRUPTED;
            break;
        }
        struct erofs_dirent *de = (void *)block;
        unsigned int nameoff = le16_to_cpu(de->nameoff);
        if (!nameoff || nameoff > size || nameoff >= blocksize || nameoff % sizeof(*de)) {
            error = -EFSCORRUPTED;
            break;
        }
        error = erofs_fill_dentries(inode, &ctx, block, de, nameoff, (unsigned int)size);
        if (error) {
            if (emitter.result)
                error = emitter.result;
            break;
        }
    }
    free(block);
    return error;
}
