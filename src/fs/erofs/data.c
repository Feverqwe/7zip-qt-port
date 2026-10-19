// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017-2018 HUAWEI, Inc.
 *             https://www.huawei.com/
 * Copyright (C) 2021, Alibaba Cloud
 */
#include "internal.h"

static int erofs_map_chunks(struct inode *inode, struct erofs_map_blocks *map)
{
	struct erofs_buf buf = __EROFS_BUF_INITIALIZER;
	struct super_block *sb = inode->i_sb;
	struct erofs_inode *vi = EROFS_I(inode);
	struct erofs_inode_chunk_index *idx;
	unsigned int unit = vi->chunkformat & EROFS_CHUNK_FORMAT_INDEXES ?
		sizeof(*idx) : EROFS_BLOCK_MAP_ENTRY_SIZE;
	erofs_blk_t addrmask = (vi->chunkformat & EROFS_CHUNK_FORMAT_48BIT) ?
		BIT_ULL(48) - 1 : BIT_ULL(32) - 1;
	u64 nr = map->m_la >> vi->chunkbits, chunksize = 1ULL << vi->chunkbits;
	erofs_off_t pos = ALIGN(erofs_iloc(inode) + vi->inode_isize +
				vi->xattr_isize, unit) + unit * nr;
	/* m_llen will be clamped to EOF in the end */
	erofs_off_t endpos = round_up(pos + 1, sb->s_blocksize);
	u64 last, addr;

	idx = erofs_read_metabuf(&buf, sb, pos, erofs_inode_in_metabox(inode));
	if (IS_ERR(idx))
		return PTR_ERR(idx);

	map->m_la = nr << vi->chunkbits;
	map->m_llen = 0;
	nr = 0;
	do {
		if (unit == EROFS_BLOCK_MAP_ENTRY_SIZE) {
			addr = le32_to_cpu(((__le32 *)idx)[nr]);
			if (addr == (u32)EROFS_NULL_ADDR)
				addr = EROFS_NULL_ADDR;
		} else {
			addr = (((u64)le16_to_cpu(idx[nr].startblk_hi) << 32) |
				le32_to_cpu(idx[nr].startblk_lo)) & addrmask;
			if (addr ^ (EROFS_NULL_ADDR & addrmask))
				addr |= (u64)(le16_to_cpu(idx[nr].device_id) &
					EROFS_SB(sb)->device_id_mask) << 48;
			else
				addr = EROFS_NULL_ADDR;
		}
		if (!nr) {
			last = addr;
			continue;
		}
		/* expand and account the prior chunk here */
		map->m_llen += chunksize;
		if (last != EROFS_NULL_ADDR)
			last += erofs_blknr(sb, chunksize);
	} while (addr == last && pos + (++nr) * unit < endpos);

	if (last != EROFS_NULL_ADDR) {
		map->m_pa = erofs_pos(sb, last & addrmask) - map->m_llen;
		map->m_deviceid = last >> 48;
		map->m_flags = EROFS_MAP_MAPPED;
	}
	if (addr == last)
		map->m_llen += chunksize;
	map->m_llen = min_t(erofs_off_t, map->m_llen,
			round_up(inode->i_size - map->m_la, sb->s_blocksize));
	erofs_put_metabuf(&buf);
	return 0;
}

int erofs_map_blocks(struct inode *inode, struct erofs_map_blocks *map)
{
	struct super_block *sb = inode->i_sb;
	struct erofs_inode *vi = EROFS_I(inode);
	bool tailinline = (vi->datalayout == EROFS_INODE_FLAT_INLINE);
	erofs_off_t pos;
	int err = 0;

	trace_erofs_map_blocks_enter(inode, map, 0);
	map->m_deviceid = 0;
	map->m_flags = 0;
	if (map->m_la >= inode->i_size)
		goto out;
	if (vi->datalayout == EROFS_INODE_CHUNK_BASED) {
		err = erofs_map_chunks(inode, map);
	} else if (tailinline || vi->startblk != EROFS_NULL_ADDR) {
		pos = erofs_pos(sb, erofs_iblks(inode) - tailinline);
		map->m_flags = EROFS_MAP_MAPPED;
		if (map->m_la < pos) {
			map->m_pa = erofs_pos(sb, vi->startblk) + map->m_la;
			map->m_llen = pos - map->m_la;
		} else {
			map->m_pa = erofs_iloc(inode) + vi->inode_isize +
				vi->xattr_isize + erofs_blkoff(sb, map->m_la);
			map->m_llen = inode->i_size - map->m_la;
			map->m_flags |= EROFS_MAP_META;
			if (erofs_blkoff(sb, map->m_pa) + map->m_llen >
			    sb->s_blocksize) {
				erofs_err(sb, "inline data across blocks @ nid %llu", vi->nid);
				return -EFSCORRUPTED;
			}
		}
	}
out:
	map->m_plen = err ? 0 : map->m_llen;
	trace_erofs_map_blocks_exit(inode, map, 0, err);
	return err;
}
