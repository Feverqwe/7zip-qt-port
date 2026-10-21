// SPDX-License-Identifier: GPL-2.0-only
/* Archive transport/lifetime boundary for the Linux classic HFS read modules. */
#include "fs/hfs/btree.h"

struct sevenzip_hfs_classic {
    struct super_block sb;
    struct hfs_sb_info sbi;
    struct hfs_find_data catalog;
    struct inode current[2];
    u32 records;
    u32 previous_parent;
    struct hfs_name previous_name;
};

int hfs_read_bytes(struct super_block *sb, u64 offset, void *data, size_t size)
{
    struct hfs_sb_info *sbi = HFS_SB(sb);
    int error;
    if (offset > sbi->size || size > sbi->size - offset)
        error = -EIO;
    else
        error = sbi->read(sbi->context, offset, data, size);
    if (error)
        sbi->error = error;
    return error;
}

int hfs_read_fork(struct inode *inode, u64 offset, void *data, size_t size)
{
    struct hfs_sb_info *sbi = HFS_SB(inode->i_sb);
    if (offset > inode->i_size || size > inode->i_size - offset)
        return -EIO;
    while (size) {
        u32 block;
        size_t within = (size_t)(offset % sbi->alloc_blksz);
        size_t length = sbi->alloc_blksz - within;
        int error = hfs_get_block(inode, (u32)(offset / sbi->alloc_blksz), &block);
        if (error)
            return error;
        if (length > size)
            length = size;
        error = hfs_read_bytes(inode->i_sb, (u64)sbi->fs_start * 512 +
                               (u64)block * sbi->alloc_blksz + within, data, length);
        if (error)
            return error;
        offset += length;
        data = (u8 *)data + length;
        size -= length;
    }
    return 0;
}

int sevenzip_hfs_classic_open(sevenzip_hfs_classic_read_fn read, void *context,
                              uint64_t size, struct sevenzip_hfs_classic **state,
                              struct sevenzip_hfs_classic_volume *volume)
{
    struct sevenzip_hfs_classic *s = calloc(1, sizeof(*s));
    int error;
    *state = NULL;
    if (!s)
        return -ENOMEM;
    s->sb.s_fs_info = &s->sbi;
    s->sbi.read = read;
    s->sbi.context = context;
    s->sbi.size = size;
    error = hfs_mdb_get(&s->sb);
    if (!error)
        error = hfs_find_init(s->sbi.cat_tree, &s->catalog);
    if (!error) {
        s->catalog.bnode = hfs_bnode_find(s->sbi.cat_tree, s->sbi.cat_tree->leaf_head);
        if (IS_ERR(s->catalog.bnode)) {
            error = PTR_ERR(s->catalog.bnode);
            s->catalog.bnode = NULL;
        } else if (s->catalog.bnode->type != HFS_NODE_LEAF || s->catalog.bnode->prev)
            error = -EIO;
        s->catalog.record = -1;
    }
    if (error) {
        sevenzip_hfs_classic_close(s);
        return error;
    }
    volume->block_size = s->sbi.alloc_blksz;
    volume->blocks = s->sbi.fs_ablocks;
    volume->free_blocks = be16_to_cpu(s->sbi.mdb.drFreeBks);
    volume->files = be32_to_cpu(s->sbi.mdb.drFilCnt);
    volume->folders = be32_to_cpu(s->sbi.mdb.drDirCnt);
    volume->created = be32_to_cpu(s->sbi.mdb.drCrDate);
    volume->modified = be32_to_cpu(s->sbi.mdb.drLsMod);
    volume->allocation_start = (u64)s->sbi.fs_start * 512;
    volume->unclean = !(s->sbi.mdb.drAtrb & cpu_to_be16(HFS_SB_ATTRIB_UNMNT));
    *state = s;
    return 0;
}

int sevenzip_hfs_classic_next(struct sevenzip_hfs_classic *s,
                              struct sevenzip_hfs_classic_item *item)
{
    struct hfs_find_data *fd = &s->catalog;
    for (;;) {
        hfs_cat_rec rec;
        u32 previous = fd->bnode->this;
        u32 parent;
        int error;
        if (s->records == fd->tree->leaf_count)
            return fd->bnode->this == fd->tree->leaf_tail &&
                   fd->record + 1 == fd->bnode->num_recs && !fd->bnode->next ? 0 : -EIO;
        if (s->records > fd->tree->leaf_count)
            return -EIO;
        error = hfs_brec_goto(fd, 1);
        if (error || s->sbi.error)
            return error ? error : s->sbi.error;
        if (fd->bnode->type != HFS_NODE_LEAF ||
            (fd->bnode->this != previous && fd->bnode->prev != previous))
            return -EIO;
        parent = be32_to_cpu(fd->key->cat.ParID);
        if (s->records && (parent < s->previous_parent ||
            (parent == s->previous_parent && hfs_strcmp(s->previous_name.name, s->previous_name.len,
                                                        fd->key->cat.CName.name, fd->key->cat.CName.len) >= 0)))
            return -EIO;
        s->previous_parent = parent;
        s->previous_name = fd->key->cat.CName;
        s->records++;
        if (fd->entrylength < 1 || (size_t)fd->entrylength > sizeof(rec))
            return -EIO;
        memset(&rec, 0, sizeof(rec));
        hfs_bnode_read(fd->bnode, &rec, (u32)fd->entryoffset, (u32)fd->entrylength);
        if (rec.type == HFS_CDR_THD || rec.type == HFS_CDR_FTH) {
            if (fd->entrylength < sizeof(struct hfs_cat_thread) || fd->key->cat.CName.len ||
                rec.thread.CName.len > HFS_NAMELEN)
                return -EIO;
            continue;
        }
        if ((rec.type == HFS_CDR_FIL && fd->entrylength < sizeof(struct hfs_cat_file)) ||
            (rec.type == HFS_CDR_DIR && fd->entrylength < sizeof(struct hfs_cat_dir)) ||
            (rec.type != HFS_CDR_FIL && rec.type != HFS_CDR_DIR) || !fd->key->cat.CName.len)
            return -EIO;
        memset(item, 0, sizeof(*item));
        memset(s->current, 0, sizeof(s->current));
        for (int resource = 0; resource < 2; resource++) {
            s->current[resource].i_sb = &s->sb;
            error = hfs_read_inode(&s->current[resource], &rec, resource);
            if (error)
                return error;
            item->size[resource] = s->current[resource].i_size;
            item->blocks[resource] = HFS_I(&s->current[resource])->alloc_blocks;
        }
        item->id = s->current[0].i_ino;
        item->mode = s->current[0].i_mode;
        item->parent = parent;
        item->name_length = (u8)hfs_mac2asc(item->name, &fd->key->cat.CName);
        if (rec.type == HFS_CDR_FIL) {
            item->created = be32_to_cpu(rec.file.CrDat);
            item->modified = be32_to_cpu(rec.file.MdDat);
            memcpy(item->finder_type, &rec.file.UsrWds.fdType, 4);
            memcpy(item->finder_creator, &rec.file.UsrWds.fdCreator, 4);
        } else {
            struct hfs_find_data thread;
            error = hfs_find_init(s->sbi.cat_tree, &thread);
            if (!error) {
                error = hfs_cat_find_brec(&s->sb, item->id, &thread);
                if (!error && (be32_to_cpu(thread.key->cat.ParID) != item->parent ||
                    hfs_strcmp(thread.key->cat.CName.name, thread.key->cat.CName.len,
                               fd->key->cat.CName.name, fd->key->cat.CName.len)))
                    error = -EIO;
                hfs_find_exit(&thread);
            }
            if (error)
                return error;
            item->created = be32_to_cpu(rec.dir.CrDat);
            item->modified = be32_to_cpu(rec.dir.MdDat);
        }
        return 1;
    }
}

int sevenzip_hfs_classic_map(struct sevenzip_hfs_classic *s, int resource,
                             uint32_t logical_block, uint32_t *physical_block)
{
    if (resource < 0 || resource > 1)
        return -EINVAL;
    return hfs_get_block(&s->current[resource], logical_block, physical_block);
}

void sevenzip_hfs_classic_close(struct sevenzip_hfs_classic *s)
{
    if (!s)
        return;
    if (s->catalog.tree)
        hfs_find_exit(&s->catalog);
    hfs_btree_close(s->sbi.cat_tree);
    hfs_btree_close(s->sbi.ext_tree);
    free(s);
}
