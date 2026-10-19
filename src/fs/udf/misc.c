// SPDX-License-Identifier: GPL-2.0-only
/*
 * misc.c
 *
 * PURPOSE
 *	Miscellaneous routines for the OSTA-UDF(tm) filesystem.
 *
 * COPYRIGHT
 *  (C) 1998 Dave Boynton
 *  (C) 1998-2004 Ben Fennema
 *  (C) 1999-2000 Stelias Computing Inc
 *
 * HISTORY
 *
 *  04/19/99 blf  partial support for reading/writing specific EA's
 */

#include "udfdecl.h"

struct buffer_head *udf_read_tagged(struct super_block *sb, udf_pblk_t block,
				    uint32_t location, uint16_t *ident)
{
	struct tag *tag_p;
	struct buffer_head *bh = NULL;
	u8 checksum;

	/* Read the block */
	if (block == 0xFFFFFFFF)
		return NULL;

	bh = sb_bread(sb, block);
	if (!bh) {
		udf_err(sb, "read failed, block=%u, location=%u\n",
			block, location);
		return NULL;
	}

	tag_p = (struct tag *)(bh->b_data);

	*ident = le16_to_cpu(tag_p->tagIdent);

	if (location != le32_to_cpu(tag_p->tagLocation)) {
		udf_debug("location mismatch block %u, tag %u != %u\n",
			  block, le32_to_cpu(tag_p->tagLocation), location);
		goto error_out;
	}

	/* Verify the tag checksum */
	checksum = udf_tag_checksum(tag_p);
	if (checksum != tag_p->tagChecksum) {
		udf_err(sb, "tag checksum failed, block %u: 0x%02x != 0x%02x\n",
			block, checksum, tag_p->tagChecksum);
		goto error_out;
	}

	/* Verify the tag version */
	if (tag_p->descVersion != cpu_to_le16(0x0002U) &&
	    tag_p->descVersion != cpu_to_le16(0x0003U)) {
		udf_err(sb, "tag version 0x%04x != 0x0002 || 0x0003, block %u\n",
			le16_to_cpu(tag_p->descVersion), block);
		goto error_out;
	}

	/* Verify the descriptor CRC */
	if (le16_to_cpu(tag_p->descCRCLength) + sizeof(struct tag) > sb->s_blocksize) {
		udf_err(sb, "block %u: CRC length %u exceeds block size\n",
			block, le16_to_cpu(tag_p->descCRCLength));
		goto error_out;
	}
	if (le16_to_cpu(tag_p->descCRC) == crc_itu_t(0,
					bh->b_data + sizeof(struct tag),
					le16_to_cpu(tag_p->descCRCLength)))
		return bh;

	udf_debug("Crc failure block %u: crc = %u, crclen = %u\n", block,
		  le16_to_cpu(tag_p->descCRC),
		  le16_to_cpu(tag_p->descCRCLength));
error_out:
	brelse(bh);
	return NULL;
}

struct buffer_head *udf_read_ptagged(struct super_block *sb,
				     struct kernel_lb_addr *loc,
				     uint32_t offset, uint16_t *ident)
{
	return udf_read_tagged(sb, udf_get_lb_pblock(sb, loc, offset),
			       loc->logicalBlockNum + offset, ident);
}

u8 udf_tag_checksum(const struct tag *t)
{
	u8 *data = (u8 *)t;
	u8 checksum = 0;
	int i;
	for (i = 0; i < sizeof(struct tag); ++i)
		if (i != 4) /* position of checksum */
			checksum += data[i];
	return checksum;
}
