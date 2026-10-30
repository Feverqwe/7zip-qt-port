// SPDX-License-Identifier: GPL-2.0-only
/*
 * directory.c
 *
 * PURPOSE
 *	Directory related functions
 *
 */

#include "formats/udf/udfdecl.h"

struct short_ad *udf_get_fileshortad(uint8_t *ptr, int maxoffset, uint32_t *offset,
			      int inc)
{
	struct short_ad *sa;

	if ((!ptr) || (!offset)) {
		pr_err("%s: invalidparms\n", __func__);
		return NULL;
	}

	if ((*offset + sizeof(struct short_ad)) > maxoffset)
		return NULL;
	else {
		sa = (struct short_ad *)ptr;
		if (sa->extLength == 0)
			return NULL;
	}

	if (inc)
		*offset += sizeof(struct short_ad);
	return sa;
}

struct long_ad *udf_get_filelongad(uint8_t *ptr, int maxoffset, uint32_t *offset, int inc)
{
	struct long_ad *la;

	if ((!ptr) || (!offset)) {
		pr_err("%s: invalidparms\n", __func__);
		return NULL;
	}

	if ((*offset + sizeof(struct long_ad)) > maxoffset)
		return NULL;
	else {
		la = (struct long_ad *)ptr;
		if (la->extLength == 0)
			return NULL;
	}

	if (inc)
		*offset += sizeof(struct long_ad);
	return la;
}


static int udf_verify_fi(struct udf_fileident_iter *iter)
{
	unsigned int len;

	if (iter->fi.descTag.tagIdent != cpu_to_le16(TAG_IDENT_FID)) {
		udf_err(iter->dir->i_sb,
			"directory (ino %llu) has entry at pos %llu with incorrect tag %x\n",
			iter->dir->i_ino, (unsigned long long)iter->pos,
			le16_to_cpu(iter->fi.descTag.tagIdent));
		return -EFSCORRUPTED;
	}
	len = udf_dir_entry_len(&iter->fi);
	if (le16_to_cpu(iter->fi.lengthOfImpUse) & 3) {
		udf_err(iter->dir->i_sb,
			"directory (ino %llu) has entry at pos %llu with unaligned length of impUse field\n",
			iter->dir->i_ino, (unsigned long long)iter->pos);
		return -EFSCORRUPTED;
	}
	/*
	 * This is in fact allowed by the spec due to long impUse field but
	 * we don't support it. If there is real media with this large impUse
	 * field, support can be added.
	 */
	if (len > 1 << iter->dir->i_blkbits) {
		udf_err(iter->dir->i_sb,
			"directory (ino %llu) has too big (%u) entry at pos %llu\n",
			iter->dir->i_ino, len, (unsigned long long)iter->pos);
		return -EFSCORRUPTED;
	}
	if (iter->pos + len > iter->dir->i_size) {
		udf_err(iter->dir->i_sb,
			"directory (ino %llu) has entry past directory size at pos %llu\n",
			iter->dir->i_ino, (unsigned long long)iter->pos);
		return -EFSCORRUPTED;
	}
	if (udf_dir_entry_len(&iter->fi) !=
	    sizeof(struct tag) + le16_to_cpu(iter->fi.descTag.descCRCLength)) {
		udf_err(iter->dir->i_sb,
			"directory (ino %llu) has entry where CRC length (%u) does not match entry length (%u)\n",
			iter->dir->i_ino,
			(unsigned)le16_to_cpu(iter->fi.descTag.descCRCLength),
			(unsigned)(udf_dir_entry_len(&iter->fi) -
							sizeof(struct tag)));
		return -EFSCORRUPTED;
	}
	return 0;
}

static int udf_copy_fi(struct udf_fileident_iter *iter)
{
	struct udf_inode_info *iinfo = UDF_I(iter->dir);
	u32 blksize = 1 << iter->dir->i_blkbits;
	u32 off, len, nameoff;
	int err;

	/* Skip copying when we are at EOF */
	if (iter->pos >= iter->dir->i_size) {
		iter->name = NULL;
		return 0;
	}
	if (iter->dir->i_size < iter->pos + sizeof(struct fileIdentDesc)) {
		udf_err(iter->dir->i_sb,
			"directory (ino %llu) has entry straddling EOF\n",
			iter->dir->i_ino);
		return -EFSCORRUPTED;
	}
	if (iinfo->i_alloc_type == ICBTAG_FLAG_AD_IN_ICB) {
		memcpy(&iter->fi, iinfo->i_data + iinfo->i_lenEAttr + iter->pos,
		       sizeof(struct fileIdentDesc));
		err = udf_verify_fi(iter);
		if (err < 0)
			return err;
		iter->name = iinfo->i_data + iinfo->i_lenEAttr + iter->pos +
			sizeof(struct fileIdentDesc) +
			le16_to_cpu(iter->fi.lengthOfImpUse);
		return 0;
	}

	off = iter->pos & (blksize - 1);
	len = min_t(u32, sizeof(struct fileIdentDesc), blksize - off);
	memcpy(&iter->fi, iter->bh[0]->b_data + off, len);
	if (len < sizeof(struct fileIdentDesc))
		memcpy((char *)(&iter->fi) + len, iter->bh[1]->b_data,
		       sizeof(struct fileIdentDesc) - len);
	err = udf_verify_fi(iter);
	if (err < 0)
		return err;

	/* Handle directory entry name */
	nameoff = off + sizeof(struct fileIdentDesc) +
				le16_to_cpu(iter->fi.lengthOfImpUse);
	if (off + udf_dir_entry_len(&iter->fi) <= blksize) {
		iter->name = iter->bh[0]->b_data + nameoff;
	} else if (nameoff >= blksize) {
		iter->name = iter->bh[1]->b_data + (nameoff - blksize);
	} else {
		iter->name = iter->namebuf;
		len = blksize - nameoff;
		memcpy(iter->name, iter->bh[0]->b_data + nameoff, len);
		memcpy(iter->name + len, iter->bh[1]->b_data,
		       iter->fi.lengthFileIdent - len);
	}
	return 0;
}

static struct buffer_head *udf_fiiter_bread_blk(struct udf_fileident_iter *iter)
{
	udf_pblk_t blk;

	blk = udf_get_lb_pblock(iter->dir->i_sb, &iter->eloc, iter->loffset);
	return sb_bread(iter->dir->i_sb, blk);
}

/*
 * Updates loffset to point to next directory block; eloc, elen & epos are
 * updated if we need to traverse to the next extent as well.
 */
static int udf_fiiter_advance_blk(struct udf_fileident_iter *iter)
{
	int8_t etype = -1;
	int err = 0;

	iter->loffset++;
	if (iter->loffset < DIV_ROUND_UP(iter->elen, 1<<iter->dir->i_blkbits))
		return 0;

	iter->loffset = 0;
	err = sevenzip_udf_next_aext(&iter->walk, iter->dir, &iter->epos,
			    &iter->eloc, &iter->elen, &etype);
	if (err < 0)
		return err;
	else if (err == 0 || etype != (EXT_RECORDED_ALLOCATED >> 30)) {
		if (iter->pos == iter->dir->i_size) {
			iter->elen = 0;
			return 0;
		}
		udf_err(iter->dir->i_sb,
			"extent after position %llu not allocated in directory (ino %llu)\n",
			(unsigned long long)iter->pos, iter->dir->i_ino);
		return -EFSCORRUPTED;
	}
	return 0;
}

static int udf_fiiter_load_bhs(struct udf_fileident_iter *iter)
{
	int blksize = 1 << iter->dir->i_blkbits;
	int off = iter->pos & (blksize - 1);
	int err;
	struct fileIdentDesc *fi;

	/* Is there any further extent we can map from? */
	if (!iter->bh[0] && iter->elen) {
		iter->bh[0] = udf_fiiter_bread_blk(iter);
		if (!iter->bh[0]) {
			err = -ENOMEM;
			goto out_brelse;
		}
		if (!buffer_uptodate(iter->bh[0])) {
			err = -EIO;
			goto out_brelse;
		}
	}
	/* There's no next block so we are done */
	if (iter->pos >= iter->dir->i_size)
		return 0;
	/* Need to fetch next block as well? */
	if (off + sizeof(struct fileIdentDesc) > blksize)
		goto fetch_next;
	fi = (struct fileIdentDesc *)(iter->bh[0]->b_data + off);
	/* Need to fetch next block to get name? */
	if (off + udf_dir_entry_len(fi) > blksize) {
fetch_next:
		err = udf_fiiter_advance_blk(iter);
		if (err)
			goto out_brelse;
		iter->bh[1] = udf_fiiter_bread_blk(iter);
		if (!iter->bh[1]) {
			err = -ENOMEM;
			goto out_brelse;
		}
		if (!buffer_uptodate(iter->bh[1])) {
			err = -EIO;
			goto out_brelse;
		}
	}
	return 0;
out_brelse:
	brelse(iter->bh[0]);
	brelse(iter->bh[1]);
	iter->bh[0] = iter->bh[1] = NULL;
	return err;
}

int udf_fiiter_init(struct udf_fileident_iter *iter, struct inode *dir,
		    int64_t pos)
{
	struct udf_inode_info *iinfo = UDF_I(dir);
	int err = 0;
	int8_t etype;

	iter->dir = dir;
	iter->bh[0] = iter->bh[1] = NULL;
	iter->pos = pos;
	iter->elen = 0;
	iter->epos.bh = NULL;
	iter->name = NULL;
	/*
	 * When directory is verified, we don't expect directory iteration to
	 * fail and it can be difficult to undo without corrupting filesystem.
	 * So just do not allow memory allocation failures here.
	 */
	iter->namebuf = kmalloc(UDF_NAME_LEN_CS0, GFP_KERNEL | __GFP_NOFAIL);
	if (!iter->namebuf)
		return -ENOMEM;

	if (iinfo->i_alloc_type == ICBTAG_FLAG_AD_IN_ICB) {
		err = udf_copy_fi(iter);
		goto out;
	}

	err = inode_bmap(dir, iter->pos >> dir->i_blkbits, &iter->epos,
			 &iter->eloc, &iter->elen, &iter->loffset, &etype);
	if (err <= 0 || etype != (EXT_RECORDED_ALLOCATED >> 30)) {
		if (pos == dir->i_size)
			return 0;
		udf_err(dir->i_sb,
			"position %llu not allocated in directory (ino %llu)\n",
			(unsigned long long)pos, dir->i_ino);
		err = -EFSCORRUPTED;
		goto out;
	}
	err = udf_fiiter_load_bhs(iter);
	if (err < 0)
		goto out;
	err = udf_copy_fi(iter);
out:
	if (err < 0)
		udf_fiiter_release(iter);
	return err;
}

int udf_fiiter_advance(struct udf_fileident_iter *iter)
{
	unsigned int oldoff, len;
	int blksize = 1 << iter->dir->i_blkbits;
	int err;

	oldoff = iter->pos & (blksize - 1);
	len = udf_dir_entry_len(&iter->fi);
	iter->pos += len;
	if (UDF_I(iter->dir)->i_alloc_type != ICBTAG_FLAG_AD_IN_ICB) {
		if (oldoff + len >= blksize) {
			brelse(iter->bh[0]);
			iter->bh[0] = NULL;
			/* Next block already loaded? */
			if (iter->bh[1]) {
				iter->bh[0] = iter->bh[1];
				iter->bh[1] = NULL;
			} else {
				err = udf_fiiter_advance_blk(iter);
				if (err < 0)
					return err;
			}
		}
		err = udf_fiiter_load_bhs(iter);
		if (err < 0)
			return err;
	}
	return udf_copy_fi(iter);
}

void udf_fiiter_release(struct udf_fileident_iter *iter)
{
	iter->dir = NULL;
	brelse(iter->epos.bh);
	iter->epos.bh = NULL;
	brelse(iter->bh[0]);
	brelse(iter->bh[1]);
	iter->bh[0] = iter->bh[1] = NULL;
	kfree(iter->namebuf);
	iter->namebuf = NULL;
}
