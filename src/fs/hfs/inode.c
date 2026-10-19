/*
 * linux/fs/hfs/inode.c
 * Copyright (C) 1995-1997 Paul H. Hargrove
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * This file may be distributed under the terms of the GNU General Public License.
 * Fork and inode read projections; VFS cache/operations and write paths omitted.
 */
#include "hfs_fs.h"

int hfs_inode_read_fork(struct inode *inode, const struct hfs_extent *ext,
                        __be32 __log_size, __be32 phys_size)
{
    u32 log_size = be32_to_cpu(__log_size);
    u32 count;
    int i;
    memcpy(HFS_I(inode)->first_extents, ext, sizeof(hfs_extent_rec));
    for (count = 0, i = 0; i < 3; i++)
        count += be16_to_cpu(ext[i].count);
    HFS_I(inode)->first_blocks = count;
    HFS_I(inode)->cached_start = 0;
    HFS_I(inode)->cached_blocks = 0;
    inode->i_size = log_size;
    HFS_I(inode)->alloc_blocks = be32_to_cpu(phys_size) / HFS_SB(inode->i_sb)->alloc_blksz;
    if (HFS_I(inode)->alloc_blocks > HFS_SB(inode->i_sb)->fs_ablocks ||
        count > HFS_I(inode)->alloc_blocks ||
        log_size > (u64)HFS_I(inode)->alloc_blocks * HFS_SB(inode->i_sb)->alloc_blksz)
        return -EIO;
    return 0;
}

int hfs_read_inode(struct inode *inode, const hfs_cat_rec *rec, int resource)
{
    HFS_I(inode)->resource = resource;
    switch (rec->type) {
    case HFS_CDR_FIL:
        if (!hfs_is_valid_cnid(be32_to_cpu(rec->file.FlNum), rec->type))
            return -EIO;
        if (!HFS_IS_RSRC(inode)) {
            if (hfs_inode_read_fork(inode, rec->file.ExtRec, rec->file.LgLen, rec->file.PyLen))
                return -EIO;
        } else {
            if (hfs_inode_read_fork(inode, rec->file.RExtRec, rec->file.RLgLen, rec->file.RPyLen))
                return -EIO;
        }
        inode->i_ino = be32_to_cpu(rec->file.FlNum);
        inode->i_mode = 0444 | 0111;
        if (!(rec->file.Flags & HFS_FIL_LOCK))
            inode->i_mode |= 0222;
        inode->i_mode &= ~0133;
        inode->i_mode |= 0100000;
        break;
    case HFS_CDR_DIR:
        if (!hfs_is_valid_cnid(be32_to_cpu(rec->dir.DirID), rec->type))
            return -EIO;
        inode->i_ino = be32_to_cpu(rec->dir.DirID);
        inode->i_size = be16_to_cpu(rec->dir.Val) + 2;
        inode->i_mode = 0040000 | (0777 & ~0022);
        break;
    default:
        return -EIO;
    }
    return 0;
}
