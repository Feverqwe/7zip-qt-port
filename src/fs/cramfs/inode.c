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

int cramfs_read_folio(struct file *file, struct folio *folio)
{
	(void)file;
	struct inode *inode = folio->mapping->host;
	u32 maxblock;
	int bytes_filled;
	void *pgdata;
	bool success = false;

	maxblock = (inode->i_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	bytes_filled = 0;
	pgdata = kmap_local_folio(folio, 0);

	if (folio->index < maxblock) {
		struct super_block *sb = inode->i_sb;
		u32 blkptr_offset = OFFSET(inode) + folio->index * 4;
		u32 block_ptr, block_start, block_len;
		bool uncompressed, direct;

		mutex_lock(&read_mutex);
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
				if (folio->index == maxblock - 1 && offset_in_page(inode->i_size))
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
			if (folio->index)
				block_start = cramfs_read_u32(sb, blkptr_offset - 4);
			/* Beware... previous ptr might be a direct ptr */
			if (unlikely(block_start & CRAMFS_BLK_FLAG_DIRECT_PTR)) {
				/* See comments on earlier code. */
				u32 prev_start = block_start;
				block_start = prev_start & ~CRAMFS_BLK_FLAGS;
				block_start <<= CRAMFS_BLK_DIRECT_PTR_SHIFT;
				if (prev_start & CRAMFS_BLK_FLAG_UNCOMPRESSED) {
					if (block_start > UINT32_MAX - PAGE_SIZE)
						goto err;
					block_start += PAGE_SIZE;
				} else {
					block_len = cramfs_read_u16(sb, block_start);
					if (block_start > UINT32_MAX - 2 - block_len)
						goto err;
					block_start += 2 + block_len;
				}
			}
			block_start &= ~CRAMFS_BLK_FLAGS;
			block_len = block_ptr - block_start;
		}

		if (block_len == 0)
			; /* hole */
		else if (unlikely(block_len > 2*PAGE_SIZE ||
				  (uncompressed && block_len > PAGE_SIZE))) {
			mutex_unlock(&read_mutex);
			pr_err("bad data blocksize %u\n", block_len);
			goto err;
		} else if (uncompressed) {
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
