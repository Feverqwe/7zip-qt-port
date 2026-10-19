// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Squashfs - a compressed read only filesystem for Linux
 *
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * block.c
 */

#include "squashfscompat.h"
#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs.h"
#include "decompressor.h"
#include "page_actor.h"

static int copy_bio_to_actor(struct bio *bio,
			     struct squashfs_page_actor *actor,
			     int offset, int req_length)
{
	void *actor_addr;
	struct bvec_iter_all iter_all = {0};
	struct bio_vec *bvec = bvec_init_iter_all(&iter_all);
	int copied_bytes = 0;
	int actor_offset = 0;

	squashfs_actor_nobuff(actor);
	actor_addr = squashfs_first_page(actor);

	if (WARN_ON_ONCE(!bio_next_segment(bio, &iter_all)))
		return 0;

	while (copied_bytes < req_length) {
		int bytes_to_copy = min_t(int, bvec->bv_len - offset,
					  PAGE_SIZE - actor_offset);

		bytes_to_copy = min_t(int, bytes_to_copy,
				      req_length - copied_bytes);
		if (!IS_ERR(actor_addr))
			memcpy((char *)actor_addr + actor_offset, (char *)bvec_virt(bvec) +
					offset, bytes_to_copy);

		actor_offset += bytes_to_copy;
		copied_bytes += bytes_to_copy;
		offset += bytes_to_copy;

		if (actor_offset >= PAGE_SIZE) {
			actor_addr = squashfs_next_page(actor);
			if (!actor_addr)
				break;
			actor_offset = 0;
		}
		if (offset >= bvec->bv_len) {
			if (!bio_next_segment(bio, &iter_all))
				break;
			offset = 0;
		}
	}
	squashfs_finish_page(actor);
	return copied_bytes;
}

int squashfs_read_data(struct super_block *sb, u64 index, int length,
		       u64 *next_index, struct squashfs_page_actor *output)
{
	struct squashfs_sb_info *msblk = sb->s_fs_info;
	struct bio *bio = NULL;
	int compressed;
	int res;
	int offset;

	if (length) {
		/*
		 * Datablock.
		 */
		compressed = SQUASHFS_COMPRESSED_BLOCK(length);
		length = SQUASHFS_COMPRESSED_SIZE_BLOCK(length);
		TRACE("Block @ 0x%llx, %scompressed size %d, src size %d\n",
			index, compressed ? "" : "un", length, output->length);
	} else {
		/*
		 * Metadata block.
		 */
		const u8 *data;
		struct bvec_iter_all iter_all = {0};
		struct bio_vec *bvec = bvec_init_iter_all(&iter_all);

		if (index + 2 > msblk->bytes_used) {
			res = -EIO;
			goto out;
		}
		res = squashfs_bio_read(sb, index, 2, &bio, &offset);
		if (res)
			goto out;

		if (WARN_ON_ONCE(!bio_next_segment(bio, &iter_all))) {
			res = -EIO;
			goto out_free_bio;
		}
		/* Extract the length of the metadata block */
		data = bvec_virt(bvec);
		length = data[offset];
		if (offset < bvec->bv_len - 1) {
			length |= data[offset + 1] << 8;
		} else {
			if (WARN_ON_ONCE(!bio_next_segment(bio, &iter_all))) {
				res = -EIO;
				goto out_free_bio;
			}
			data = bvec_virt(bvec);
			length |= data[0] << 8;
		}
		bio_free_pages(bio);
		bio_uninit(bio);
		kfree(bio);

		compressed = SQUASHFS_COMPRESSED(length);
		length = SQUASHFS_COMPRESSED_SIZE(length);
		index += 2;

		TRACE("Block @ 0x%llx, %scompressed size %d\n", index - 2,
		      compressed ? "" : "un", length);
	}
	if (length <= 0 || length > output->length ||
			(index + length) > msblk->bytes_used) {
		res = -EIO;
		goto out;
	}

	if (next_index)
		*next_index = index + length;

	res = squashfs_bio_read(sb, index, length, &bio, &offset);
	if (res)
		goto out;

	if (compressed) {
		if (!msblk->stream) {
			res = -EIO;
			goto out_free_bio;
		}
		res = msblk->thread_ops->decompress(msblk, bio, offset, length, output);
	} else {
		res = copy_bio_to_actor(bio, output, offset, length);
	}

out_free_bio:
	bio_free_pages(bio);
	bio_uninit(bio);
	kfree(bio);
out:
	if (res < 0) {
		ERROR("Failed to read block 0x%llx: %d\n", index, res);
		if (msblk->panic_on_errors)
			panic("squashfs read failed");
	}

	return res;
}
