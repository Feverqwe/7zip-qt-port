/*
 *  linux/fs/hfs/extent.c
 *
 * Copyright (C) 1995-1997  Paul H. Hargrove
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * This file may be distributed under the terms of the GNU General Public License.
 *
 * This file contains the functions related to the extents B-tree.
 */

#include "btree.h"

static void hfs_ext_build_key(hfs_btree_key *key, u32 cnid, u16 block, u8 type)
{
	key->key_len = 7;
	key->ext.FkType = type;
	key->ext.FNum = cpu_to_be32(cnid);
	key->ext.FABN = cpu_to_be16(block);
}

int hfs_ext_keycmp(const btree_key *key1, const btree_key *key2)
{
	__be32 fnum1, fnum2;
	__be16 block1, block2;

	fnum1 = key1->ext.FNum;
	fnum2 = key2->ext.FNum;
	if (fnum1 != fnum2)
		return be32_to_cpu(fnum1) < be32_to_cpu(fnum2) ? -1 : 1;
	if (key1->ext.FkType != key2->ext.FkType)
		return key1->ext.FkType < key2->ext.FkType ? -1 : 1;

	block1 = key1->ext.FABN;
	block2 = key2->ext.FABN;
	if (block1 == block2)
		return 0;
	return be16_to_cpu(block1) < be16_to_cpu(block2) ? -1 : 1;
}

u16 hfs_ext_find_block(struct hfs_extent *ext, u16 off)
{
	int i;
	u16 count;

	for (i = 0; i < 3; ext++, i++) {
		count = be16_to_cpu(ext->count);
		if (off < count)
			return be16_to_cpu(ext->block) + off;
		off -= count;
	}
	/* panic? */
	return 0;
}

static int hfs_ext_block_count(struct hfs_extent *ext)
{
	int i;
	u16 count = 0;

	for (i = 0; i < 3; ext++, i++)
		count += be16_to_cpu(ext->count);
	return count;
}

static inline int __hfs_ext_read_extent(struct hfs_find_data *fd, struct hfs_extent *extent,
					u32 cnid, u32 block, u8 type)
{
	int res;

	hfs_ext_build_key(fd->search_key, cnid, block, type);
	fd->key->ext.FNum = 0;
	res = hfs_brec_find(fd);
	if (res && res != -ENOENT)
		return res;
	if (fd->key->ext.FNum != fd->search_key->ext.FNum ||
	    fd->key->ext.FkType != fd->search_key->ext.FkType)
		return -ENOENT;
	if (fd->entrylength != sizeof(hfs_extent_rec))
		return -EIO;
	hfs_bnode_read(fd->bnode, extent, fd->entryoffset, sizeof(hfs_extent_rec));
	return 0;
}

/* Archive allocation mapping validates the full record before asking the
 * original hfs_ext_find_block to select a block (whose fallback value is 0). */
static int hfs_extent_record_valid(struct inode *inode, struct hfs_extent *ext, u32 *blocks)
{
    struct hfs_sb_info *sbi = HFS_SB(inode->i_sb);
    u32 total = 0;
    for (unsigned i = 0; i < 3; i++) {
        u32 start = be16_to_cpu(ext[i].block);
        u32 count = be16_to_cpu(ext[i].count);
        if (count && (start >= sbi->fs_ablocks || count > sbi->fs_ablocks - start))
            return -EIO;
        total += count;
    }
    if (total > sbi->fs_ablocks)
        return -EIO;
    *blocks = total;
    return 0;
}

int hfs_ext_read_extent(struct inode *inode, u16 block)
{
    struct hfs_find_data fd;
    int res;
    if (block >= HFS_I(inode)->cached_start &&
        block < HFS_I(inode)->cached_start + HFS_I(inode)->cached_blocks)
        return 0;
    res = hfs_find_init(HFS_SB(inode->i_sb)->ext_tree, &fd);
    if (!res) {
        res = __hfs_ext_read_extent(&fd, HFS_I(inode)->cached_extents, inode->i_ino,
                                    block, HFS_IS_RSRC(inode) ? HFS_FK_RSRC : HFS_FK_DATA);
        if (!res) {
            u32 blocks;
            HFS_I(inode)->cached_start = be16_to_cpu(fd.key->ext.FABN);
            res = hfs_extent_record_valid(inode, HFS_I(inode)->cached_extents, &blocks);
            if (!res) {
                HFS_I(inode)->cached_blocks = hfs_ext_block_count(HFS_I(inode)->cached_extents);
                if (HFS_I(inode)->cached_blocks != blocks ||
                    HFS_I(inode)->cached_start > block ||
                    block - HFS_I(inode)->cached_start >= blocks)
                    res = -EIO;
            }
        }
        if (res)
            HFS_I(inode)->cached_start = HFS_I(inode)->cached_blocks = 0;
        hfs_find_exit(&fd);
    }
    return res;
}

/* Read side of hfs_get_block. The archive asks for allocation-block mappings,
 * so sector/page offsets and the create/write branch stay in the caller. */
int hfs_get_block(struct inode *inode, u32 ablock, u32 *dblock)
{
    u32 count;
    int res;
    if (ablock >= HFS_I(inode)->alloc_blocks)
        return -EIO;
    res = hfs_extent_record_valid(inode, HFS_I(inode)->first_extents, &count);
    if (res || count != HFS_I(inode)->first_blocks)
        return -EIO;
    if (ablock < HFS_I(inode)->first_blocks) {
        *dblock = hfs_ext_find_block(HFS_I(inode)->first_extents, (u16)ablock);
        return 0;
    }
    res = hfs_ext_read_extent(inode, (u16)ablock);
    if (res)
        return res;
    *dblock = hfs_ext_find_block(HFS_I(inode)->cached_extents,
                                (u16)(ablock - HFS_I(inode)->cached_start));
    return 0;
}
