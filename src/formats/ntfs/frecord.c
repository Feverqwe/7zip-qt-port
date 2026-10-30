// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 */

#include "formats/ntfs/ntfslinkcompat.h"

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
    if (!size || bits < 12 || bits > 16 || !read) return NULL;
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
    /* ni_read_frame rejects unknown compression before reading the ADS. */
    switch (frame_size) {
    case 0x1000:
    case 0x2000:
    case 0x4000:
    case 0x8000:
        break;
    default:
        return -EOPNOTSUPP;
    }
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

enum REPARSE_SIGN ni_parse_reparse(struct ntfs_inode *ni, struct ATTRIB *attr,
				   struct REPARSE_DATA_BUFFER *buffer)
{
	const struct REPARSE_DATA_BUFFER *rp = NULL;
	u8 bits;
	u16 len;
	const struct NTFS_COMPRESS_REPARSE *cmpr;

	ni = ni->base;
	/* Try to estimate reparse point. */
	if (!attr->non_res) {
		rp = resident_data_ex(attr, sizeof(struct REPARSE_DATA_BUFFER));
	} else if (le64_to_cpu(attr->nres.data_size) >=
		   sizeof(struct REPARSE_DATA_BUFFER)) {
		struct runs_tree run;

		run_init(&run);

		if (!attr_load_runs_vcn(ni, ATTR_REPARSE, NULL, 0, &run, 0) &&
		    !ntfs_read_run_nb(ni->mi.sbi, &run, 0, buffer,
				      sizeof(struct REPARSE_DATA_BUFFER),
				      NULL)) {
			rp = buffer;
		}

		run_close(&run);
	}

	if (!rp)
		return REPARSE_NONE;

	len = le16_to_cpu(rp->ReparseDataLength);
	switch (rp->ReparseTag) {
	case (IO_REPARSE_TAG_MICROSOFT | IO_REPARSE_TAG_SYMBOLIC_LINK):
		break; /* Symbolic link. */
	case IO_REPARSE_TAG_MOUNT_POINT:
		break; /* Mount points and junctions. */
	case IO_REPARSE_TAG_SYMLINK:
		break;
	case IO_REPARSE_TAG_COMPRESS:
		/*
		 * WOF - Windows Overlay Filter - Used to compress files with
		 * LZX/Xpress.
		 *
		 * Unlike native NTFS file compression, the Windows
		 * Overlay Filter supports only read operations. This means
		 * that it doesn't need to sector-align each compressed chunk,
		 * so the compressed data can be packed more tightly together.
		 * If you open the file for writing, the WOF just decompresses
		 * the entire file, turning it back into a plain file.
		 *
		 * Ntfs3 driver decompresses the entire file only on write or
		 * change size requests.
		 */

		cmpr = &rp->CompressReparseBuffer;
		if (len < sizeof(*cmpr) ||
		    cmpr->WofVersion != WOF_CURRENT_VERSION ||
		    cmpr->WofProvider != WOF_PROVIDER_SYSTEM ||
		    cmpr->ProviderVer != WOF_PROVIDER_CURRENT_VERSION) {
			return REPARSE_NONE;
		}

		switch (cmpr->CompressionFormat) {
		case WOF_COMPRESSION_XPRESS4K:
			bits = 0xc; // 4k
			break;
		case WOF_COMPRESSION_XPRESS8K:
			bits = 0xd; // 8k
			break;
		case WOF_COMPRESSION_XPRESS16K:
			bits = 0xe; // 16k
			break;
		case WOF_COMPRESSION_LZX32K:
			bits = 0xf; // 32k
			break;
		default:
			bits = 0x10; // 64k
			break;
		}
		ni_set_ext_compress_bits(ni, bits);
		return REPARSE_COMPRESSED;

	case IO_REPARSE_TAG_DEDUP:
		ni->ni_flags |= NI_FLAG_DEDUPLICATED;
		return REPARSE_DEDUPLICATED;

	default:
		if (rp->ReparseTag & IO_REPARSE_TAG_NAME_SURROGATE)
			break;

		return REPARSE_NONE;
	}

	if (buffer != rp)
		memcpy(buffer, rp, sizeof(struct REPARSE_DATA_BUFFER));

	/* Looks like normal symlink. */
	return REPARSE_LINK;
}
