// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 HUAWEI, Inc.
 *             https://www.huawei.com/
 * Copyright (C) 2024 Alibaba Cloud
 */
#include "formats/erofs/compress.h"

const char *z_erofs_fixup_insize(struct z_erofs_decompress_req *rq,
				 const char *padbuf, unsigned int padbufsize)
{
	const char *padend;

	padend = memchr_inv(padbuf, 0, padbufsize);
	if (!padend)
		return "compressed data start not found";
	rq->inputsize -= padend - padbuf;
	rq->pageofs_in += padend - padbuf;
	return NULL;
}

static const char *z_erofs_transform_plain(struct z_erofs_decompress_req *rq,
					   struct page **pagepool)
{
	const unsigned int nrpages_in = rq->inpages, nrpages_out = rq->outpages;
	const unsigned int bs = rq->sb->s_blocksize;
	unsigned int cur = 0, ni = 0, no, pi, po, insz, cnt;
	u8 *kin;

	if (rq->outputsize > rq->inputsize)
		return ERR_PTR(-EOPNOTSUPP);
	if (rq->alg == Z_EROFS_COMPRESSION_INTERLACED) {
		cur = bs - (rq->pageofs_out & (bs - 1));
		DBG_BUGON(rq->pageofs_in & (bs - 1));
		pi = (rq->pageofs_in + rq->inputsize - cur) & ~PAGE_MASK;
		cur = min(cur, rq->outputsize);
		if (cur && rq->out[0]) {
			kin = kmap_local_page(rq->in[nrpages_in - 1]);
			if (rq->out[0] == rq->in[nrpages_in - 1])
				memmove(kin + rq->pageofs_out, kin + pi, cur);
			else
				memcpy_to_page(rq->out[0], rq->pageofs_out,
					       kin + pi, cur);
			kunmap_local(kin);
		}
		rq->outputsize -= cur;
	}

	for (; rq->outputsize; rq->pageofs_in = 0, cur += insz, ni++) {
		insz = min(PAGE_SIZE - rq->pageofs_in, rq->outputsize);
		rq->outputsize -= insz;
		if (!rq->in[ni])
			continue;
		kin = kmap_local_page(rq->in[ni]);
		pi = 0;
		do {
			no = (rq->pageofs_out + cur + pi) >> PAGE_SHIFT;
			po = (rq->pageofs_out + cur + pi) & ~PAGE_MASK;
			DBG_BUGON(no >= nrpages_out);
			cnt = min(insz - pi, PAGE_SIZE - po);
			if (rq->out[no] == rq->in[ni])
				memmove(kin + po,
					kin + rq->pageofs_in + pi, cnt);
			else if (rq->out[no])
				memcpy_to_page(rq->out[no], po,
					       kin + rq->pageofs_in + pi, cnt);
			pi += cnt;
		} while (pi < insz);
		kunmap_local(kin);
	}
	DBG_BUGON(ni > nrpages_in);
	return NULL;
}

int erofs_decode_extent(struct inode *inode, struct erofs_map_blocks *map,
                         const void *input, void *output)
{
	struct super_block *sb = inode->i_sb;
	struct z_erofs_decompress_req rq = {
		.sb = sb,
		.inputsize = (unsigned int)map->m_plen,
		.outputsize = (unsigned int)map->m_llen,
		.alg = map->m_algorithmformat,
		.partial_decoding = !EROFS_MAP_FULL(map->m_flags),
	};
	if (rq.alg < Z_EROFS_COMPRESSION_MAX) {
		if (!sb->decode)
			return -EOPNOTSUPP;
		rq.pageofs_in = (unsigned int)erofs_blkoff(sb, map->m_pa);
		const char *reason = z_erofs_fixup_insize(&rq, input,
			min(rq.inputsize, sb->s_blocksize - rq.pageofs_in));
		if (reason)
			return -EFSCORRUPTED;
		unsigned int dictionary = rq.alg == Z_EROFS_COMPRESSION_ZSTD ?
			EROFS_SB(sb)->zstd_dict_size : EROFS_SB(sb)->lzma_dict_size;
		return sb->decode(sb->context, rq.alg, dictionary,
			(const u8 *)input + map->m_plen - rq.inputsize, rq.inputsize,
			output, rq.outputsize, rq.partial_decoding);
	}
	if (rq.alg >= Z_EROFS_COMPRESSION_RUNTIME_MAX || rq.outputsize > rq.inputsize)
		return -EFSCORRUPTED;
	rq.pageofs_out = (unsigned int)(map->m_la & (PAGE_SIZE - 1));
	rq.inpages = (rq.inputsize + PAGE_SIZE - 1) >> PAGE_SHIFT;
	rq.outpages = (rq.outputsize + rq.pageofs_out + PAGE_SIZE - 1) >> PAGE_SHIFT;
	struct page *pages = calloc(rq.inpages + rq.outpages, sizeof(*pages));
	struct page **pointers = calloc(rq.inpages + rq.outpages, sizeof(*pointers));
	u8 *out = calloc(rq.outpages, PAGE_SIZE);
	if (!pages || !pointers || !out) {
		free(pages); free(pointers); free(out);
		return -ENOMEM;
	}
	for (unsigned int i = 0; i < rq.inpages + rq.outpages; ++i) {
		pages[i].data = i < rq.inpages ? (u8 *)input + (size_t)i * PAGE_SIZE :
			out + (size_t)(i - rq.inpages) * PAGE_SIZE;
		pointers[i] = &pages[i];
	}
	rq.in = pointers;
	rq.out = pointers + rq.inpages;
	const char *reason = z_erofs_transform_plain(&rq, NULL);
	if (!reason)
		memcpy(output, out + rq.pageofs_out, (size_t)map->m_llen);
	free(pages); free(pointers); free(out);
	return reason ? -EFSCORRUPTED : 0;
}


static int z_erofs_load_lz4_config(struct super_block *sb,
			    struct erofs_super_block *dsb, void *data, int size)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);
	struct z_erofs_lz4_cfgs *lz4 = data;
	u16 distance;

	if (lz4) {
		if (size < sizeof(struct z_erofs_lz4_cfgs)) {
			erofs_err(sb, "invalid lz4 cfgs, size=%u", size);
			return -EINVAL;
		}
		sbi->lz4.max_pclusterblks = le16_to_cpu(lz4->max_pclusterblks);
		if (!sbi->lz4.max_pclusterblks) {
			sbi->lz4.max_pclusterblks = 1;	/* reserved case */
		} else if (sbi->lz4.max_pclusterblks >
			   erofs_blknr(sb, Z_EROFS_PCLUSTER_MAX_SIZE)) {
			erofs_err(sb, "too large lz4 pclusterblks %u",
				  sbi->lz4.max_pclusterblks);
			return -EINVAL;
		}
	} else {
		distance = le16_to_cpu(dsb->u1.lz4_max_distance);
		if (!distance && !erofs_sb_has_lz4_0padding(sbi))
			return 0;
		sbi->lz4.max_pclusterblks = 1;
		sbi->available_compr_algs = 1 << Z_EROFS_COMPRESSION_LZ4;
	}
	return 0;
}


struct z_erofs_decompressor {
	int (*config)(struct super_block *, struct erofs_super_block *, void *, int);
};
static const struct z_erofs_decompressor z_erofs_configs[] = {
	{z_erofs_load_lz4_config},
	{z_erofs_load_lzma_config},
	{z_erofs_load_deflate_config},
	{z_erofs_load_zstd_config},
};
static const struct z_erofs_decompressor *z_erofs_decomp[] = {
	&z_erofs_configs[0], &z_erofs_configs[1], &z_erofs_configs[2], &z_erofs_configs[3]
};
int z_erofs_parse_cfgs(struct super_block *sb, struct erofs_super_block *dsb)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);
	struct erofs_buf buf = __EROFS_BUF_INITIALIZER;
	unsigned long algs, alg;
	erofs_off_t offset;
	int size, ret = 0;

	if (!erofs_sb_has_compr_cfgs(sbi))
		return z_erofs_load_lz4_config(sb, dsb, NULL, 0);

	algs = le16_to_cpu(dsb->u1.available_compr_algs);
	sbi->available_compr_algs = algs;
	if (algs & ~Z_EROFS_ALL_COMPR_ALGS) {
		erofs_err(sb, "unidentified algorithms %lx, please upgrade kernel",
			  algs & ~Z_EROFS_ALL_COMPR_ALGS);
		return -EOPNOTSUPP;
	}

	(void)erofs_init_metabuf(&buf, sb, false);
	offset = EROFS_SUPER_OFFSET + sbi->sb_size;
	for_each_set_bit(alg, &algs, Z_EROFS_COMPRESSION_MAX) {
		const struct z_erofs_decompressor *dec = z_erofs_decomp[alg];
		void *data;

		data = erofs_read_metadata(sb, &buf, &offset, &size);
		if (IS_ERR(data)) {
			ret = PTR_ERR(data);
			break;
		}

		if (dec && dec->config) {
			ret = dec->config(sb, dsb, data, size);
		} else {
			erofs_err(sb, "algorithm %ld isn't enabled on this kernel",
				  alg);
			ret = -EOPNOTSUPP;
		}
		kfree(data);
		if (ret)
			break;
	}
	erofs_put_metabuf(&buf);
	return ret;
}
