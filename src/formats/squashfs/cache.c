// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Squashfs - a compressed read only filesystem for Linux
 *
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * cache.c
 */

/*
 * Blocks in Squashfs are compressed.  To avoid repeatedly decompressing
 * recently accessed data Squashfs uses two small metadata and fragment caches.
 *
 * This file implements a generic cache implementation used for both caches,
 * plus functions layered ontop of the generic cache implementation to
 * access the metadata and fragment caches.
 *
 * To avoid out of memory and fragmentation issues with vmalloc the cache
 * uses sequences of kmalloced PAGE_SIZE buffers.
 *
 * It should be noted that the cache is not used for file datablocks, these
 * are decompressed and cached in the page-cache in the normal way.  The
 * cache is only used to temporarily cache fragment and metadata blocks
 * which have been read as as a result of a metadata (i.e. inode or
 * directory) or fragment access.  Because metadata and fragments are packed
 * together into blocks (to gain greater compression) the read of a particular
 * piece of metadata or fragment will retrieve other metadata/fragments which
 * have been packed with it, these because of locality-of-reference may be read
 * in the near future. Temporarily caching them ensures they are available for
 * near future access without requiring an additional read and decompress.
 */

#include "formats/squashfs/squashfscompat.h"
#include "formats/squashfs/squashfs_fs.h"
#include "formats/squashfs/squashfs_fs_sb.h"
#include "formats/squashfs/squashfs.h"
#include "formats/squashfs/page_actor.h"

int squashfs_copy_data(void *buffer, struct squashfs_cache_entry *entry,
		int offset, int length)
{
	int remaining = length;

	if (length == 0 || offset < 0)
		return 0;
	else if (buffer == NULL)
		return min(length, entry->length - offset);

	while (offset < entry->length) {
		void *buff = (char *)entry->data[offset / PAGE_SIZE]
				+ (offset % PAGE_SIZE);
		int bytes = min_t(int, entry->length - offset,
				PAGE_SIZE - (offset % PAGE_SIZE));

		if (bytes >= remaining) {
			memcpy(buffer, buff, remaining);
			remaining = 0;
			break;
		}

		memcpy(buffer, buff, bytes);
		buffer = (char *)buffer + bytes;
		remaining -= bytes;
		offset += bytes;
	}

	return length - remaining;
}


/*
 * Read length bytes from metadata position <block, offset> (block is the
 * start of the compressed block on disk, and offset is the offset into
 * the block once decompressed).  Data is packed into consecutive blocks,
 * and length bytes may require reading more than one block.
 */
int squashfs_read_metadata(struct super_block *sb, void *buffer,
		u64 *block, int *offset, int length)
{
	struct squashfs_sb_info *msblk = sb->s_fs_info;
	int bytes, res = length;
	struct squashfs_cache_entry *entry;

	TRACE("Entered squashfs_read_metadata [%llx:%x]\n", *block, *offset);

	if (unlikely(length < 0))
		return -EIO;

	if (unlikely(*offset < 0 || *offset >= SQUASHFS_METADATA_SIZE))
		return -EIO;

	while (length) {
		entry = squashfs_cache_get(sb, msblk->block_cache, *block, 0);
		if (entry->error) {
			res = entry->error;
			goto error;
		} else if (*offset >= entry->length) {
			res = -EIO;
			goto error;
		}

		bytes = squashfs_copy_data(buffer, entry, *offset, length);
		if (buffer)
			buffer = (char *)buffer + bytes;
		length -= bytes;
		*offset += bytes;

		if (*offset == entry->length) {
			*block = entry->next_index;
			*offset = 0;
		}

		squashfs_cache_put(entry);
	}

	return res;

error:
	squashfs_cache_put(entry);
	return res;
}


void *squashfs_read_table(struct super_block *sb, u64 block, int length)
{
	int pages;
	int i, res;
	void *table, **data;
	struct squashfs_page_actor *actor;

	/* Validate the archive range before allocating. */
	if (length <= 0 ||
	    block > (u64)sb->s_fs_info->bytes_used ||
	    (unsigned int)length > (u64)sb->s_fs_info->bytes_used - block)
		return ERR_PTR(-EINVAL);
	pages = ((length - 1) >> PAGE_SHIFT) + 1;
	table = kmalloc(length, GFP_KERNEL);
	if (table == NULL)
		return ERR_PTR(-ENOMEM);

	data = kcalloc(pages, sizeof(void *), GFP_KERNEL);
	if (data == NULL) {
		res = -ENOMEM;
		goto failed;
	}

	actor = squashfs_page_actor_init(data, pages, length);
	if (actor == NULL) {
		res = -ENOMEM;
		goto failed2;
	}

	for (i = 0; i < pages; i++)
		data[i] = (char *)table + (size_t)i * PAGE_SIZE;

	/* Raw tables have no compressed-size field.  Keep each request below
	 * the encoded flag bit instead of limiting the complete table. */
	for (i = 0; i < pages; i++) {
		actor->buffer = data + i;
		actor->pages = 1;
		actor->length = min(length - i * PAGE_SIZE, PAGE_SIZE);
		res = squashfs_read_data(sb, block + (u64)i * PAGE_SIZE,
			actor->length | SQUASHFS_COMPRESSED_BIT_BLOCK, NULL, actor);
		if (res != actor->length) {
			if (res >= 0)
				res = -EIO;
			break;
		}
	}

	kfree(data);
	kfree(actor);

	if (res < 0)
		goto failed;

	return table;

failed2:
	kfree(data);
failed:
	kfree(table);
	return ERR_PTR(res);
}
