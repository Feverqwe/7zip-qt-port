/*
 * linux/fs/hfs/mdb.c
 * Copyright (C) 1995-1997 Paul H. Hargrove
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * This file may be distributed under the terms of the GNU General Public License.
 * Read-only MDB path; the archive supplies an already selected volume and I/O.
 */
#include "btree.h"

int hfs_mdb_get(struct super_block *sb)
{
    struct hfs_sb_info *sbi = HFS_SB(sb);
    struct hfs_mdb *mdb = &sbi->mdb;
    u32 size;
    int err = hfs_read_bytes(sb, (u64)HFS_MDB_BLK * HFS_SECTOR_SIZE, mdb, sizeof(*mdb));
    if (err)
        return err;
    if (mdb->drSigWord != cpu_to_be16(HFS_SUPER_MAGIC))
        return -EINVAL;
    sbi->alloc_blksz = size = be32_to_cpu(mdb->drAlBlkSiz);
    if (!size || (size & (HFS_SECTOR_SIZE - 1)))
        return -EIO;
    /* Archive reads use byte offsets; hardware block alignment, sessions and
     * partition selection belong to the caller. Non-power-of-two alloc sizes
     * remain valid, as in the original MDB path. */
    sbi->fs_ablocks = be16_to_cpu(mdb->drNmAlBlks);
    sbi->fs_div = sbi->alloc_blksz / HFS_SECTOR_SIZE;
    sbi->fs_start = be16_to_cpu(mdb->drAlBlSt);
    if (!sbi->fs_ablocks ||
        (u64)sbi->fs_start * HFS_SECTOR_SIZE + (u64)sbi->fs_ablocks * size > sbi->size)
        return -EIO;
    sbi->ext_tree = hfs_btree_open(sb, HFS_EXT_CNID, hfs_ext_keycmp);
    if (!sbi->ext_tree)
        return sbi->error ? sbi->error : -EIO;
    sbi->cat_tree = hfs_btree_open(sb, HFS_CAT_CNID, hfs_cat_keycmp);
    if (!sbi->cat_tree)
        return sbi->error ? sbi->error : -EIO;
    return 0;
}
