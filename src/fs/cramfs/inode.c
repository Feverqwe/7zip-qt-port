/*
 * Compressed rom filesystem for Linux.
 *
 * Copyright (C) 1999 Linus Torvalds.
 *
 * This file is released under the GPL.
 */

/*
 * These are the VFS interfaces to the compressed rom filesystem.
 * The actual compression is based on zlib, see the other files.
 */

#include "internal.h"

/* The original folio pointer mapping is shared by reading and extent inspection. */
int cramfs_map_inode_block(struct inode *inode, u32 index, struct cramfs_extent *extent)
{
	struct super_block *sb = inode->i_sb;
	u32 maxblock = ((uint64_t)inode->i_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	if (index >= maxblock || OFFSET(inode) > UINT32_MAX - maxblock * 4)
		return -1;
	u32 blkptr_offset = OFFSET(inode) + index * 4;
	u32 block_ptr, block_start, block_len;
	bool uncompressed, direct;

	block_ptr = cramfs_read_u32(sb, blkptr_offset);
	uncompressed = (block_ptr & CRAMFS_BLK_FLAG_UNCOMPRESSED);
	direct = (block_ptr & CRAMFS_BLK_FLAG_DIRECT_PTR);
	block_ptr &= ~CRAMFS_BLK_FLAGS;

	if (direct) {
		/*
		 * The block pointer is an absolute start pointer,
		 * shifted by 2 bits. The size is included in the
		 * first 2 bytes of the data block when compressed,
		 * or PAGE_SIZE otherwise.
		 */
		block_start = block_ptr << CRAMFS_BLK_DIRECT_PTR_SHIFT;
		if (uncompressed) {
			block_len = PAGE_SIZE;
			/* if last block: cap to file length */
			if (index == maxblock - 1 && offset_in_page(inode->i_size))
				block_len =
					offset_in_page(inode->i_size);
		} else {
			block_len = cramfs_read_u16(sb, block_start);
			block_start += 2;
		}
	} else {
		/*
		 * The block pointer indicates one past the end of
		 * the current block (start of next block). If this
		 * is the first block then it starts where the block
		 * pointer table ends, otherwise its start comes
		 * from the previous block's pointer.
		 */
		block_start = OFFSET(inode) + maxblock * 4;
		if (index)
			block_start = cramfs_read_u32(sb, blkptr_offset - 4);
		/* Beware... previous ptr might be a direct ptr */
		if (unlikely(block_start & CRAMFS_BLK_FLAG_DIRECT_PTR)) {
			/* See comments on earlier code. */
			u32 prev_start = block_start;
			block_start = prev_start & ~CRAMFS_BLK_FLAGS;
			block_start <<= CRAMFS_BLK_DIRECT_PTR_SHIFT;
			if (prev_start & CRAMFS_BLK_FLAG_UNCOMPRESSED) {
				if (block_start > UINT32_MAX - PAGE_SIZE)
					return -1;
				block_start += PAGE_SIZE;
			} else {
				block_len = cramfs_read_u16(sb, block_start);
				if (block_start > UINT32_MAX - 2 - block_len)
					return -1;
				block_start += 2 + block_len;
			}
		}
		block_start &= ~CRAMFS_BLK_FLAGS;
		block_len = block_ptr - block_start;
	}
	if (sb->error || block_len > 2 * PAGE_SIZE ||
	    (uncompressed && block_len > PAGE_SIZE) ||
	    (block_len && (block_start > sb->source->size ||
	                  block_len > sb->source->size - block_start)))
		return -1;
	extent->start = block_start;
	extent->length = block_len;
	extent->uncompressed = uncompressed;
	return 0;
}

int cramfs_read_folio(struct file *file, struct folio *folio)
{
	(void)file;
	struct inode *inode = folio->mapping->host;
	u32 maxblock;
	int bytes_filled;
	void *pgdata;
	bool success = false;

	maxblock = ((uint64_t)inode->i_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	bytes_filled = 0;
	pgdata = kmap_local_folio(folio, 0);

	if (folio->index < maxblock) {
		struct super_block *sb = inode->i_sb;
		struct cramfs_extent extent;
		mutex_lock(&read_mutex);
		if (cramfs_map_inode_block(inode, folio->index, &extent)) {
			mutex_unlock(&read_mutex);
			goto err;
		}
		u32 block_start = extent.start, block_len = extent.length;
		bool uncompressed = extent.uncompressed;

		if (block_len == 0)
			; /* hole */
		else if (uncompressed) {
			memcpy(pgdata,
			       cramfs_read(sb, block_start, block_len),
			       block_len);
			bytes_filled = block_len;
		} else {
			bytes_filled = sb->source->inflate(sb->source->context, pgdata,
				 PAGE_SIZE,
				 cramfs_read(sb, block_start, block_len),
				 block_len);
		}
		mutex_unlock(&read_mutex);
		if (unlikely(bytes_filled < 0))
			goto err;
	}

	memset((char *)pgdata + bytes_filled, 0, PAGE_SIZE - bytes_filled);
	flush_dcache_folio(folio);

	success = true;
err:
	kunmap_local(pgdata);
	folio_end_read(folio, success);
	return 0;
}
