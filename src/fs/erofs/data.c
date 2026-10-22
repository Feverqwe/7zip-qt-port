// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017-2018 HUAWEI, Inc.
 *             https://www.huawei.com/
 * Copyright (C) 2021, Alibaba Cloud
 */
#include "internal.h"

void erofs_unmap_metabuf(struct erofs_buf *buf)
{
	if (!buf->base)
		return;
	kunmap_local(buf->base);
	buf->base = NULL;
}

void erofs_put_metabuf(struct erofs_buf *buf)
{
	if (!buf->page)
		return;
	erofs_unmap_metabuf(buf);
	folio_put(page_folio(buf->page));
	buf->page = NULL;
}

void *erofs_bread(struct erofs_buf *buf, erofs_off_t offset, bool need_kmap)
{
	struct super_block *sb = buf->sb;
	if (!sb && buf->mapping)
		sb = buf->sb = buf->mapping->i_sb;
	if (!sb)
		return ERR_PTR(-EINVAL);
	if (offset >= (buf->mapping ? (u64)buf->mapping->i_size : sb->image_size))
		return ERR_PTR(-EIO);
	if (sb->poll && sb->poll(sb->context))
		return ERR_PTR(-ECANCELED);
	pgoff_t index = (buf->off + offset) >> PAGE_SHIFT;
	struct folio *folio = NULL;

	if (buf->page) {
		folio = page_folio(buf->page);
		if (folio_file_page(folio, index) != buf->page)
			erofs_unmap_metabuf(buf);
	}
	if (!folio || !folio_contains(folio, index)) {
		erofs_put_metabuf(buf);
		folio = erofs_read_cache_folio(buf, index);
		if (IS_ERR(folio))
			return folio;
	}
	buf->page = folio_file_page(folio, index);
	if (!need_kmap)
		return NULL;
	if (!buf->base)
		buf->base = kmap_local_page(buf->page);
	return (char *)buf->base + ((buf->off + offset) & ~PAGE_MASK);
}

void *erofs_read_metabuf(struct erofs_buf *buf, struct super_block *sb,
			 erofs_off_t offset, bool in_metabox)
{
	int err;

	err = erofs_init_metabuf(buf, sb, in_metabox);
	if (err)
		return ERR_PTR(err);
	return erofs_bread(buf, offset, true);
}


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

static void erofs_fill_from_devinfo(struct erofs_map_dev *map,
		struct super_block *sb, struct erofs_device_info *dif)
{
	map->m_sb = sb;
	map->m_dif = dif;
	/* Device/block-file handles are supplied by the archive transport. */
}

int erofs_map_dev(struct super_block *sb, struct erofs_map_dev *map)
{
	/* The immutable stream table replaces the kernel IDR and its read lock. */
	struct erofs_sb_info *devs = EROFS_SB(sb);
	struct erofs_device_info *dif;
	erofs_off_t startoff;
	unsigned int id;

	erofs_fill_from_devinfo(map, sb, &EROFS_SB(sb)->dif0);
	if (map->m_deviceid) {
		dif = map->m_deviceid <= devs->extra_devices ?
			&devs->devices[map->m_deviceid - 1] : NULL;
		if (!dif)
			return -ENODEV;
		if (devs->flatdev) {
			if (check_add_overflow(map->m_pa,
					       erofs_pos(sb, dif->uniaddr), &map->m_pa))
				return -EFSCORRUPTED;
			return 0;
		}
		erofs_fill_from_devinfo(map, sb, dif);
	} else if (devs->extra_devices && !devs->flatdev) {
		for (id = 0; id < devs->extra_devices; ++id) {
			dif = &devs->devices[id];
			if (!dif->uniaddr)
				continue;

			startoff = erofs_pos(sb, dif->uniaddr);
			if (map->m_pa >= startoff &&
			    erofs_blknr(sb, map->m_pa - startoff) < dif->blocks) {
				map->m_pa -= startoff;
				erofs_fill_from_devinfo(map, sb, dif);
				break;
			}
		}
	}
	return 0;
}
