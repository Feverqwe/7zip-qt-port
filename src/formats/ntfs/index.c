// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 */

#include "formats/ntfs/ntfssecuritycompat.h"
#include "formats/ntfs/ntfsdirectory.h"

static const struct INDEX_NAMES {
	const __le16 *name;
	u8 name_len;
} s_index_names[INDEX_MUTEX_TOTAL] = {
	{ I30_NAME, ARRAY_SIZE(I30_NAME) }, { SII_NAME, ARRAY_SIZE(SII_NAME) },
	{ SDH_NAME, ARRAY_SIZE(SDH_NAME) }, { SO_NAME, ARRAY_SIZE(SO_NAME) },
	{ SQ_NAME, ARRAY_SIZE(SQ_NAME) },   { SR_NAME, ARRAY_SIZE(SR_NAME) },
};


static int cmp_uint(const void *key1, size_t l1, const void *key2, size_t l2,
		    const void *data)
{
	const u32 *k1 = key1;
	const u32 *k2 = key2;

	if (l2 < sizeof(u32))
		return -1;

	if (*k1 < *k2)
		return -1;
	if (*k1 > *k2)
		return 1;
	return 0;
}

static inline NTFS_CMP_FUNC get_cmp_func(const struct INDEX_ROOT *root)
{
    /* Read-only $SII specialization; filename/SDH/other collations omitted. */
    return root->type == ATTR_ZERO && root->rule == NTFS_COLLATION_TYPE_UINT ? cmp_uint : NULL;
}

static bool index_hdr_check(const struct INDEX_HDR *hdr, u32 bytes)
{
	const bool has_subnode = hdr_has_subnode(hdr);
	const u16 min_size =
		sizeof(struct NTFS_DE) + (has_subnode ? sizeof(u64) : 0);
	u32 end = le32_to_cpu(hdr->used);
	u32 tot = le32_to_cpu(hdr->total);
	u32 off = le32_to_cpu(hdr->de_off);
	const struct NTFS_DE *e;

	if (off < sizeof(*hdr) || !IS_ALIGNED(off, 8) || tot > bytes || end > tot ||
	    size_add(off, min_size) > end) {
		/* incorrect index buffer. */
		return false;
	}

	/* Ensure every key stays inside its entry before lookup walks it. */
	e = (const struct NTFS_DE *)((const u8 *)hdr + off);
	for (;;) {
		if (size_add(off, sizeof(*e)) > end)
			return false;

		u16 e_size = le16_to_cpu(e->size);
		u16 key_size = le16_to_cpu(e->key_size);
		u16 data_size;

		if (!IS_ALIGNED(e_size, 8) || e_size < min_size ||
		    de_has_vcn(e) != has_subnode) {
			/* incorrect index entry. */
			return false;
		}

		if (size_add(off, e_size) > end)
			return false;

		if (de_is_last(e)) {
			if (key_size)
				return false;

			break;
		}

		data_size = e_size - min_size;
		if (key_size > data_size)
			return false;

		off += e_size;
		e = (const struct NTFS_DE *)((const u8 *)hdr + off);
	}

	return true;
}

static bool index_buf_check(const struct INDEX_BUFFER *ib, u32 bytes,
			    const CLST *vbn)
{
	const struct NTFS_RECORD_HEADER *rhdr = &ib->rhdr;
	u16 fo = le16_to_cpu(rhdr->fix_off);
	u16 fn = le16_to_cpu(rhdr->fix_num);

	if (bytes <= offsetof(struct INDEX_BUFFER, ihdr) ||
	    rhdr->sign != NTFS_INDX_SIGNATURE ||
	    fo < sizeof(struct INDEX_BUFFER)
	    /* Check index buffer vbn. */
	    || (vbn && *vbn != le64_to_cpu(ib->vbn)) || (fo % sizeof(short)) ||
	    fo + fn * sizeof(short) >= bytes ||
	    fn != ((bytes >> SECTOR_SHIFT) + 1)) {
		/* incorrect index buffer. */
		return false;
	}

	return index_hdr_check(&ib->ihdr,
			       bytes - offsetof(struct INDEX_BUFFER, ihdr));
}

void fnd_clear(struct ntfs_fnd *fnd)
{
	int i;

	for (i = fnd->level - 1; i >= 0; i--) {
		struct indx_node *n = fnd->nodes[i];

		if (!n)
			continue;

		put_indx_node(n);
		fnd->nodes[i] = NULL;
	}
	fnd->level = 0;
	fnd->root_de = NULL;
}

static int fnd_push(struct ntfs_fnd *fnd, struct indx_node *n,
		    struct NTFS_DE *e)
{
	int i = fnd->level;

	if (i < 0 || i >= ARRAY_SIZE(fnd->nodes))
		return -EINVAL;
	fnd->nodes[i] = n;
	fnd->de[i] = e;
	fnd->level += 1;
	return 0;
}

static struct NTFS_DE *hdr_find_e(const struct ntfs_index *indx,
				  const struct INDEX_HDR *hdr, const void *key,
				  size_t key_len, const void *ctx, int *diff,
				  NTFS_CMP_FUNC cmp)
{
	struct NTFS_DE *e, *found = NULL;
	int min_idx = 0, mid_idx, max_idx = 0;
	int diff2;
	int table_size = 8;
	u32 e_size, e_key_len;
	u32 end = le32_to_cpu(hdr->used);
	u32 off = le32_to_cpu(hdr->de_off);
	u32 total = le32_to_cpu(hdr->total);
	u16 offs[128];

fill_table:
	if (end > total)
		return NULL;

	if (size_add(off, sizeof(struct NTFS_DE)) > end)
		return NULL;

	e = Add2Ptr(hdr, off);
	e_size = le16_to_cpu(e->size);

	if (e_size < sizeof(struct NTFS_DE) || off + e_size > end)
		return NULL;

	if (!de_is_last(e)) {
		offs[max_idx] = off;
		off += e_size;

		max_idx++;
		if (max_idx < table_size)
			goto fill_table;

		max_idx--;
	}

binary_search:
	e_key_len = le16_to_cpu(e->key_size);

	/* Validate key_size fits within the entry data area. */
	if (e_key_len > le16_to_cpu(e->size) - sizeof(struct NTFS_DE))
		return NULL;

	diff2 = (*cmp)(key, key_len, e + 1, e_key_len, ctx);
	if (diff2 > 0) {
		if (found) {
			min_idx = mid_idx + 1;
		} else {
			if (de_is_last(e))
				return NULL;

			max_idx = 0;
			table_size = min(table_size * 2, (int)ARRAY_SIZE(offs));
			goto fill_table;
		}
	} else if (diff2 < 0) {
		if (found)
			max_idx = mid_idx - 1;
		else
			max_idx--;

		found = e;
	} else {
		*diff = 0;
		return e;
	}

	if (min_idx > max_idx) {
		*diff = -1;
		return found;
	}

	mid_idx = (min_idx + max_idx) >> 1;
	e = Add2Ptr(hdr, offs[mid_idx]);

	goto binary_search;
}

int indx_init(struct ntfs_index *indx, struct ntfs_sb_info *sbi,
	      const struct ATTRIB *attr, enum index_mutex_classed type)
{
	u32 t32;
	const struct INDEX_ROOT *root = resident_data(attr);

	t32 = le32_to_cpu(attr->res.data_size);
	if (t32 <= offsetof(struct INDEX_ROOT, ihdr) ||
	    !index_hdr_check(&root->ihdr,
			     t32 - offsetof(struct INDEX_ROOT, ihdr))) {
		goto out;
	}

	/* Check root fields. */
	if (!root->index_block_clst)
		goto out;

	indx->type = type;
	indx->idx2vbn_bits = __ffs(root->index_block_clst);

	t32 = le32_to_cpu(root->index_block_size);
	indx->index_bits = blksize_bits(t32);

	/* Check index record size. */
	if (t32 < sbi->cluster_size) {
		/* Index record is smaller than a cluster, use 512 blocks. */
		if (t32 != root->index_block_clst * SECTOR_SIZE)
			goto out;

		/* Check alignment to a cluster. */
		if ((sbi->cluster_size >> SECTOR_SHIFT) &
		    (root->index_block_clst - 1)) {
			goto out;
		}

		indx->vbn2vbo_bits = SECTOR_SHIFT;
	} else {
		/* Index record must be a multiple of cluster size. */
		if (t32 != root->index_block_clst << sbi->cluster_bits)
			goto out;

		indx->vbn2vbo_bits = sbi->cluster_bits;
	}

	init_rwsem(&indx->run_lock);

	return 0;

out:
	ntfs_set_state(sbi, NTFS_DIRTY_DIRTY);
	return -EINVAL;
}

struct INDEX_ROOT *indx_get_root(struct ntfs_index *indx, struct ntfs_inode *ni,
				 struct ATTRIB **attr, struct mft_inode **mi)
{
	struct ATTR_LIST_ENTRY *le = NULL;
	struct ATTRIB *a;
	const struct INDEX_NAMES *in = &s_index_names[indx->type];
	struct INDEX_ROOT *root;

	a = ni_find_attr(ni, NULL, &le, ATTR_ROOT, in->name, in->name_len, NULL,
			 mi);
	if (!a)
		return NULL;

	if (attr)
		*attr = a;

	root = resident_data_ex(a, sizeof(struct INDEX_ROOT));

	/* length check */
	if (root &&
	    offsetof(struct INDEX_ROOT, ihdr) + le32_to_cpu(root->ihdr.used) >
		    le32_to_cpu(a->res.data_size)) {
		return NULL;
	}

	return root;
}

int indx_read_ra(struct ntfs_index *indx, struct ntfs_inode *ni, CLST vbn,
		 struct indx_node **node, struct file_ra_state *ra)
{
	int err;
	struct INDEX_BUFFER *ib;
	struct ntfs_sb_info *sbi = ni->mi.sbi;
	struct runs_tree *run = &indx->alloc_run;
	struct rw_semaphore *lock = &indx->run_lock;
	u64 vbo = (u64)vbn << indx->vbn2vbo_bits;
	u32 bytes = 1u << indx->index_bits;
	struct indx_node *in = *node;
	const struct INDEX_NAMES *name;

	if (vbn > (UINT64_MAX >> indx->vbn2vbo_bits))
		return -EINVAL;

	if (!in) {
		in = kzalloc_obj(struct indx_node, GFP_NOFS);
		if (!in)
			return -ENOMEM;
	} else {
		nb_put(&in->nb);
	}

	ib = in->index;
	if (!ib) {
		ib = kmalloc(bytes, GFP_NOFS);
		if (!ib) {
			err = -ENOMEM;
			goto out;
		}
	}

	down_read(lock);
	err = ntfs_read_bh_ra(sbi, run, vbo, &ib->rhdr, bytes, &in->nb, ra);
	up_read(lock);
	if (!err)
		goto ok;

	if (err == -E_NTFS_FIXUP)
		goto ok;

	if (err != -ENOENT)
		goto out;

	name = &s_index_names[indx->type];
	down_write(lock);
	err = attr_load_runs_range(ni, ATTR_ALLOC, name->name, name->name_len,
				   run, vbo, vbo + bytes);
	up_write(lock);
	if (err)
		goto out;

	down_read(lock);
	err = ntfs_read_bh_ra(sbi, run, vbo, &ib->rhdr, bytes, &in->nb, ra);
	up_read(lock);
	if (err == -E_NTFS_FIXUP)
		goto ok;

	if (err)
		goto out;

ok:
	if (!index_buf_check(ib, bytes, &vbn)) {
		_ntfs_bad_inode(&ni->vfs_inode);
		err = -EINVAL;
		goto out;
	}

	if (err == -E_NTFS_FIXUP) {
		ntfs_write_bh(sbi, &ib->rhdr, &in->nb, 0);
		err = 0;
	}

	/* check for index header length */
	if (offsetof(struct INDEX_BUFFER, ihdr) + le32_to_cpu(ib->ihdr.used) >
	    bytes) {
		err = -EINVAL;
		goto out;
	}

	in->index = ib;
	*node = in;

out:
	if (err == -E_NTFS_CORRUPT) {
		_ntfs_bad_inode(&ni->vfs_inode);
		err = -EINVAL;
	}

	if (ib != in->index)
		kfree(ib);

	if (*node != in) {
		nb_put(&in->nb);
		kfree(in);
	}

	return err;
}

int indx_find(struct ntfs_index *indx, struct ntfs_inode *ni,
	      const struct INDEX_ROOT *root, const void *key, size_t key_len,
	      const void *ctx, int *diff, struct NTFS_DE **entry,
	      struct ntfs_fnd *fnd)
{
	int err;
	struct NTFS_DE *e;
	struct indx_node *node;
	NTFS_CMP_FUNC cmp;

	if (!root)
		root = indx_get_root(&ni->dir, ni, NULL, NULL);

	if (!root) {
		/* Should not happen. */
		return -EINVAL;
	}

	cmp = get_cmp_func(root);
	if (unlikely(!cmp)) {
		WARN_ON_ONCE(1);
		return -EINVAL;
	}

	/* Check cache. */
	e = fnd->level ? fnd->de[fnd->level - 1] : fnd->root_de;
	if (e && !de_is_last(e) &&
	    !(*cmp)(key, key_len, e + 1, le16_to_cpu(e->key_size), ctx)) {
		*entry = e;
		*diff = 0;
		return 0;
	}

	/* Soft finder reset. */
	fnd_clear(fnd);

	/* Lookup entry that is <= to the search value. */
	e = hdr_find_e(indx, &root->ihdr, key, key_len, ctx, diff, cmp);
	if (!e)
		return -EINVAL;

	fnd->root_de = e;

	for (;;) {
		node = NULL;
		if (*diff >= 0 || !de_has_vcn_ex(e))
			break;

		/* Read next level. */
		err = indx_read(indx, ni, de_get_vbn(e), &node);
		if (err) {
			/* io error? */
			return err;
		}

		/* Lookup entry that is <= to the search value. */
		e = hdr_find_e(indx, &node->index->ihdr, key, key_len, ctx,
			       diff, cmp);
		if (!e) {
			put_indx_node(node);
			return -EINVAL;
		}

		err = fnd_push(fnd, node, e);

		if (err) {
			put_indx_node(node);
			return err;
		}
	}

	*entry = e;
	return 0;
}

/* scan_nres_bitmap's physical run/block services are supplied by the archive's
 * validated logical bitmap stream. Keep its block scan, valid/data bounds and
 * scan_for_used callback; no full-bitmap allocation or native buffer_head. */
static int scan_nres_bitmap(struct ntfs_inode *ni, struct ATTRIB *bitmap,
                           struct ntfs_index *indx, size_t from,
                           bool (*fn)(const void *, u32, u32, size_t *), size_t *ret)
{
    const struct sevenzip_ntfs_directory_input *input = ni->directory_input;
    unsigned long buffer[4096 / sizeof(unsigned long)];
    u64 vbo = (from >> 3) & ~(u64)(sizeof(buffer) - 1);
    u32 first = (u32)(from & (sizeof(buffer) * 8 - 1));
    (void)bitmap; (void)indx;
    *ret = MINUS_ONE_T;
    while (vbo < input->bitmap_valid) {
        const size_t bytes = (size_t)min(sizeof(buffer), input->bitmap_size - vbo);
        const size_t valid = (size_t)min(bytes, input->bitmap_valid - vbo);
        int err;
        memset(buffer, 0, sizeof(buffer));
        err = input->read(input->opaque, 0, vbo, buffer, valid);
        if (err)
            return err;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        for (size_t i = 0; i < ARRAY_SIZE(buffer); ++i)
            buffer[i] = sizeof(unsigned long) == 8 ? get_unaligned_le64(&buffer[i]) :
                                                   get_unaligned_le32(&buffer[i]);
#endif
        if ((*fn)(buffer, first, (u32)(bytes * 8), ret)) {
            *ret += (size_t)vbo * 8;
            return 0;
        }
        vbo += bytes;
        first = 0;
    }
    return 0;
}

static bool scan_for_used(const void *buf, u32 bit, u32 bits, size_t *ret)
{
	size_t pos = _find_next_bit(buf, bits, bit);

	if (pos >= bits)
		return false;
	*ret = pos;
	return true;
}

int indx_used_bit(struct ntfs_index *indx, struct ntfs_inode *ni, size_t *bit)
{
	struct ATTRIB *b;
	struct ATTR_LIST_ENTRY *le = NULL;
	size_t from = *bit;
	const struct INDEX_NAMES *in = &s_index_names[indx->type];

	b = ni_find_attr(ni, NULL, &le, ATTR_BITMAP, in->name, in->name_len,
			 NULL, NULL);

	if (!b)
		return -ENOENT;

	*bit = MINUS_ONE_T;

	/* Both attribute storage kinds use the same bounded stream service. */
	return scan_nres_bitmap(ni, b, indx, from, &scan_for_used, bit);
}
