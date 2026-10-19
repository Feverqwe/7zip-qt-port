// SPDX-License-Identifier: GPL-2.0-only
/*
 * inode.c
 *
 * PURPOSE
 *  Inode handling routines for the OSTA-UDF(tm) filesystem.
 *
 * COPYRIGHT
 *  (C) 1998 Dave Boynton
 *  (C) 1998-2004 Ben Fennema
 *  (C) 1999-2000 Stelias Computing Inc
 *
 * HISTORY
 *
 *  10/04/98 dgb  Added rudimentary directory functions
 *  10/07/98      Fully working udf_block_map! It works!
 *  11/25/98      bmap altered to better support extents
 *  12/06/98 blf  partition support in udf_iget, udf_block_map
 *                and udf_read_inode
 *  12/12/98      rewrote udf_block_map to handle next extents and descs across
 *                block boundaries (which is not actually allowed)
 *  12/20/98      added support for strategy 4096
 *  03/07/99      rewrote udf_block_map (again)
 *                New funcs, inode_bmap, udf_next_aext
 *  04/19/99      Support for writing device EA's for major/minor #
 */

#include "udfdecl.h"

#define UDF_MAX_INDIR_EXTS 16

int udf_next_aext(struct inode *inode, struct extent_position *epos,
		  struct kernel_lb_addr *eloc, uint32_t *elen, int8_t *etype,
		  int inc)
{
	unsigned int indirections = 0;
	int ret = 0;
	udf_pblk_t block;

	while (1) {
		ret = udf_current_aext(inode, epos, eloc, elen,
				       etype, inc);
		if (ret <= 0)
			return ret;
		if (*etype != (EXT_NEXT_EXTENT_ALLOCDESCS >> 30))
			return ret;

		if (++indirections > UDF_MAX_INDIR_EXTS) {
			udf_err(inode->i_sb,
				"too many indirect extents in inode %llu\n",
				inode->i_ino);
			return -EFSCORRUPTED;
		}

		epos->block = *eloc;
		epos->offset = sizeof(struct allocExtDesc);
		brelse(epos->bh);
		block = udf_get_lb_pblock(inode->i_sb, &epos->block, 0);
		epos->bh = sb_bread(inode->i_sb, block);
		if (!epos->bh) {
			udf_debug("reading block %u failed!\n", block);
			return -EIO;
		}
	}
}

int udf_current_aext(struct inode *inode, struct extent_position *epos,
		     struct kernel_lb_addr *eloc, uint32_t *elen, int8_t *etype,
		     int inc)
{
	int alen;
	uint8_t *ptr;
	struct short_ad *sad;
	struct long_ad *lad;
	struct udf_inode_info *iinfo = UDF_I(inode);

	if (!epos->bh) {
		if (!epos->offset)
			epos->offset = udf_file_entry_alloc_offset(inode);
		ptr = iinfo->i_data + epos->offset -
			udf_file_entry_alloc_offset(inode) +
			iinfo->i_lenEAttr;
		alen = udf_file_entry_alloc_offset(inode) +
							iinfo->i_lenAlloc;
	} else {
		struct allocExtDesc *header =
			(struct allocExtDesc *)epos->bh->b_data;

		if (!epos->offset)
			epos->offset = sizeof(struct allocExtDesc);
		ptr = epos->bh->b_data + epos->offset;
		if (check_add_overflow(sizeof(struct allocExtDesc),
				le32_to_cpu(header->lengthAllocDescs), &alen))
			return -1;

		if (alen > epos->bh->b_size)
			return -1;
	}

	switch (iinfo->i_alloc_type) {
	case ICBTAG_FLAG_AD_SHORT:
		sad = udf_get_fileshortad(ptr, alen, &epos->offset, inc);
		if (!sad)
			return 0;
		*etype = le32_to_cpu(sad->extLength) >> 30;
		eloc->logicalBlockNum = le32_to_cpu(sad->extPosition);
		eloc->partitionReferenceNum =
				iinfo->i_location.partitionReferenceNum;
		*elen = le32_to_cpu(sad->extLength) & UDF_EXTENT_LENGTH_MASK;
		break;
	case ICBTAG_FLAG_AD_LONG:
		lad = udf_get_filelongad(ptr, alen, &epos->offset, inc);
		if (!lad)
			return 0;
		*etype = le32_to_cpu(lad->extLength) >> 30;
		*eloc = lelb_to_cpu(lad->extLocation);
		*elen = le32_to_cpu(lad->extLength) & UDF_EXTENT_LENGTH_MASK;
		break;
	default:
		udf_debug("alloc_type = %u unsupported\n", iinfo->i_alloc_type);
		return -EINVAL;
	}

	if (eloc->partitionReferenceNum >= UDF_SB(inode->i_sb)->s_partitions) {
		udf_debug("invalid partition reference %u (partitions %u)\n",
			  eloc->partitionReferenceNum,
			  UDF_SB(inode->i_sb)->s_partitions);
		return -EFSCORRUPTED;
	}

	return 1;
}
