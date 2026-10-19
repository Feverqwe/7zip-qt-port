// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 */

#include "ntfswofcompat.h"

static int decompress_lzx_xpress(struct ntfs_sb_info *sbi, const char *cmpr,
				 size_t cmpr_size, void *unc, size_t unc_size,
				 u32 frame_size)
{
	int err;
	void *ctx;

	if (cmpr_size == unc_size) {
		/* Frame not compressed. */
		memcpy(unc, cmpr, unc_size);
		return 0;
	}

	err = 0;
	if (frame_size == 0x8000) {
		mutex_lock(&sbi->compress.mtx_lzx);
		/* LZX: Frame compressed. */
		ctx = sbi->compress.lzx;
		if (!ctx) {
			/* Lazy initialize LZX decompress context. */
			ctx = lzx_allocate_decompressor();
			if (!ctx) {
				err = -ENOMEM;
				goto out1;
			}

			sbi->compress.lzx = ctx;
		}

		if (lzx_decompress(ctx, cmpr, cmpr_size, unc, unc_size)) {
			/* Treat all errors as "invalid argument". */
			err = -EINVAL;
		}
out1:
		mutex_unlock(&sbi->compress.mtx_lzx);
	} else {
		/* XPRESS: Frame compressed. */
		mutex_lock(&sbi->compress.mtx_xpress);
		ctx = sbi->compress.xpress;
		if (!ctx) {
			/* Lazy initialize Xpress decompress context. */
			ctx = xpress_allocate_decompressor();
			if (!ctx) {
				err = -ENOMEM;
				goto out2;
			}

			sbi->compress.xpress = ctx;
		}

		if (xpress_decompress(ctx, cmpr, cmpr_size, unc, unc_size)) {
			/* Treat all errors as "invalid argument". */
			err = -EINVAL;
		}
out2:
		mutex_unlock(&sbi->compress.mtx_xpress);
	}
	return err;
}

/* Archive adapter: each stream owns its original decoder and offset-page cache. */
struct sevenzip_ntfs_wof {
    struct ntfs_inode ni;
    struct ntfs_sb_info sbi;
    struct ATTRIB attr;
    struct runs_tree run;
    u64 size, ads_size;
    unsigned bits;
    u8 input[32768];
};

struct sevenzip_ntfs_wof *sevenzip_ntfs_wof_create(uint64_t size, uint64_t ads_size,
                                                unsigned bits,
                                                sevenzip_ntfs_read_fn read, void *opaque)
{
    struct sevenzip_ntfs_wof *w;
    if (!size || bits < 12 || bits > 15 || !read) return NULL;
    w = calloc(1, sizeof(*w));
    if (!w) return NULL;
    w->size = size; w->ads_size = ads_size; w->bits = bits;
    w->sbi.read = read; w->sbi.opaque = opaque;
    w->ni.mi.sbi = &w->sbi; w->ni.vfs_inode.i_size = size;
    w->attr.non_res = 1;
    put_unaligned_le64(ads_size, &w->attr.nres.data_size);
    return w;
}

void sevenzip_ntfs_wof_destroy(struct sevenzip_ntfs_wof *w)
{
    if (!w) return;
    lzx_free_decompressor(w->sbi.compress.lzx);
    xpress_free_decompressor(w->sbi.compress.xpress);
    free(w->ni.file.offs_folio);
    free(w);
}

int sevenzip_ntfs_wof_read_frame(struct sevenzip_ntfs_wof *w, uint64_t frame, void *output)
{
    u64 frames = (w->size - 1) >> w->bits;
    u32 frame_size = 1u << w->bits, disk_size, unc_size;
    u64 offset;
    int err;
    if (frame > frames) return -EINVAL;
    /* ni_read_frame uses the offset preceding the frame and the final ADS size. */
    err = attr_wof_frame_info(&w->ni, &w->attr, &w->run, frame, frames,
                              (u8)w->bits, &disk_size, &offset);
    if (err) return err;
    if (offset > w->ads_size) return -EINVAL;
    unc_size = frame == frames ? 1 + ((w->size - 1) & (frame_size - 1)) : frame_size;
    if (frame == frames) {
        if (w->ads_size - offset > frame_size) return -EINVAL;
        disk_size = (u32)(w->ads_size - offset);
    }
    if (!disk_size || disk_size > frame_size || disk_size > w->ads_size - offset)
        return -EINVAL;
    err = ntfs_read_run(&w->sbi, &w->run, w->input, offset, disk_size);
    if (err) return err;
    err = decompress_lzx_xpress(&w->sbi, (const char *)w->input, disk_size,
                                output, unc_size, frame_size);
    if (!err && unc_size < frame_size)
        memset((u8 *)output + unc_size, 0, frame_size - unc_size);
    return err;
}
