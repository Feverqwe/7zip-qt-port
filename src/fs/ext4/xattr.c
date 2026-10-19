// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/ext4/xattr.c
 *
 * Copyright (C) 2001-2003 Andreas Gruenbacher, <agruen@suse.de>
 *
 * Fix by Harrison Xing <harrison@mountainviewdata.com>.
 * Ext4 code with a lot of help from Eric Jarman <ejarman@acm.org>.
 * Extended attributes for symlinks and special files added per
 *  suggestion of Luka Renko <luka.renko@hermes.si>.
 * xattr consolidation Copyright (c) 2004 James Morris <jmorris@redhat.com>,
 *  Red Hat Inc.
 * ea-in-inode support by Alex Tomas <alex@clusterfs.com> aka bzzz
 *  and Andreas Gruenbacher <agruen@suse.de>.
 */

/*
 * Extended attributes are stored directly in inodes (on file systems with
 * inodes bigger than 128 bytes) and on additional disk blocks. The i_file_acl
 * field contains the block number if an inode uses an additional block. All
 * attributes must fit in the inode and one additional block. Blocks that
 * contain the identical set of attributes may be shared among several inodes.
 * Identical blocks are detected by keeping a cache of blocks that have
 * recently been accessed.
 *
 * The attributes in inodes and on blocks have a different header; the entries
 * are stored in the same format:
 *
 *   +------------------+
 *   | header           |
 *   | entry 1          | |
 *   | entry 2          | | growing downwards
 *   | entry 3          | v
 *   | four null bytes  |
 *   | . . .            |
 *   | value 1          | ^
 *   | value 3          | | growing upwards
 *   | value 2          | |
 *   +------------------+
 *
 * The header is followed by multiple entry descriptors. In disk blocks, the
 * entry descriptors are kept sorted. In inodes, they are unsorted. The
 * attribute values are aligned to the end of the block in no specific order.
 *
 * Locking strategy
 * ----------------
 * EXT4_I(inode)->i_file_acl is protected by EXT4_I(inode)->xattr_sem.
 * EA blocks are only changed if they are exclusive to an inode, so
 * holding xattr_sem also means that nothing but the EA block's reference
 * count can change. Multiple writers to the same block are synchronized
 * by the buffer lock.
 */

#include "ext4.h"
#include "xattr.h"

static __le32 ext4_xattr_block_csum(struct inode *inode,
				    sector_t block_nr,
				    struct ext4_xattr_header *hdr)
{
	struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
	__u32 csum;
	__le64 dsk_block_nr = cpu_to_le64(block_nr);
	__u32 dummy_csum = 0;
	int offset = offsetof(struct ext4_xattr_header, h_checksum);

	csum = ext4_chksum(sbi->s_csum_seed, (__u8 *)&dsk_block_nr,
			   sizeof(dsk_block_nr));
	csum = ext4_chksum(csum, (__u8 *)hdr, offset);
	csum = ext4_chksum(csum, (__u8 *)&dummy_csum, sizeof(dummy_csum));
	offset += sizeof(dummy_csum);
	csum = ext4_chksum(csum, (__u8 *)hdr + offset,
			   EXT4_BLOCK_SIZE(inode->i_sb) - offset);

	return cpu_to_le32(csum);
}

static int ext4_xattr_block_csum_verify(struct inode *inode,
					struct buffer_head *bh)
{
	struct ext4_xattr_header *hdr = BHDR(bh);
	int ret = 1;

	if (ext4_has_feature_metadata_csum(inode->i_sb)) {
		lock_buffer(bh);
		ret = (hdr->h_checksum == ext4_xattr_block_csum(inode,
							bh->b_blocknr, hdr));
		unlock_buffer(bh);
	}
	return ret;
}

static int
check_xattrs(struct inode *inode, struct buffer_head *bh,
	     struct ext4_xattr_entry *entry, void *end, void *value_start,
	     const char *function, unsigned int line)
{
	struct ext4_xattr_entry *e = entry;
	int err = -EFSCORRUPTED;
	char *err_str;

	if (bh) {
		if (BHDR(bh)->h_magic != cpu_to_le32(EXT4_XATTR_MAGIC) ||
		    BHDR(bh)->h_blocks != cpu_to_le32(1)) {
			err_str = "invalid header";
			goto errout;
		}
		if (buffer_verified(bh))
			return 0;
		if (!ext4_xattr_block_csum_verify(inode, bh)) {
			err = -EFSBADCRC;
			err_str = "invalid checksum";
			goto errout;
		}
	} else {
		struct ext4_xattr_ibody_header *header = value_start;

		header -= 1;
		if ((char *)end - (char *)header < sizeof(*header) + sizeof(u32)) {
			err_str = "in-inode xattr block too small";
			goto errout;
		}
		if (header->h_magic != cpu_to_le32(EXT4_XATTR_MAGIC)) {
			err_str = "bad magic number in in-inode xattr";
			goto errout;
		}
	}

	/* Find the end of the names list */
	while (!IS_LAST_ENTRY(e)) {
		struct ext4_xattr_entry *next = EXT4_XATTR_NEXT(e);
		if ((char *)next + sizeof(u32) > (char *)end) {
			err_str = "e_name out of bounds";
			goto errout;
		}
		if (strnlen(e->e_name, e->e_name_len) != e->e_name_len) {
			err_str = "bad e_name length";
			goto errout;
		}
		e = next;
	}

	/* Check the values */
	while (!IS_LAST_ENTRY(entry)) {
		u32 size = le32_to_cpu(entry->e_value_size);
		unsigned long ea_ino = le32_to_cpu(entry->e_value_inum);

		if (!ext4_has_feature_ea_inode(inode->i_sb) && ea_ino) {
			err_str = "ea_inode specified without ea_inode feature enabled";
			goto errout;
		}
		if (ea_ino && ((ea_ino == EXT4_ROOT_INO) ||
			       !ext4_valid_inum(inode->i_sb, ea_ino))) {
			err_str = "invalid ea_ino";
			goto errout;
		}
		if (ea_ino && !size) {
			err_str = "invalid size in ea xattr";
			goto errout;
		}
		if (size > EXT4_XATTR_SIZE_MAX) {
			err_str = "e_value size too large";
			goto errout;
		}

		if (size != 0 && entry->e_value_inum == 0) {
			u16 offs = le16_to_cpu(entry->e_value_offs);
			void *value;

			/*
			 * The value cannot overlap the names, and the value
			 * with padding cannot extend beyond 'end'.  Check both
			 * the padded and unpadded sizes, since the size may
			 * overflow to 0 when adding padding.
			 */
			if (offs > (char *)end - (char *)value_start) {
				err_str = "e_value out of bounds";
				goto errout;
			}
			value = (char *)value_start + offs;
			if ((char *)value < (char *)e + sizeof(u32) ||
			    size > (char *)end - (char *)value ||
			    EXT4_XATTR_SIZE(size) > (char *)end - (char *)value) {
				err_str = "overlapping e_value ";
				goto errout;
			}
		}
		entry = EXT4_XATTR_NEXT(entry);
	}
	if (bh)
		set_buffer_verified(bh);
	return 0;

errout:
	if (bh)
		__ext4_error_inode(inode, function, line, 0, -err,
				   "corrupted xattr block %llu: %s",
				   (unsigned long long) bh->b_blocknr,
				   err_str);
	else
		__ext4_error_inode(inode, function, line, 0, -err,
				   "corrupted in-inode xattr: %s", err_str);
	return err;
}

int
__ext4_xattr_check_block(struct inode *inode, struct buffer_head *bh,
			 const char *function, unsigned int line)
{
	return check_xattrs(inode, bh, BFIRST(bh), bh->b_data + bh->b_size,
			    bh->b_data, function, line);
}

int
__xattr_check_inode(struct inode *inode, struct ext4_xattr_ibody_header *header,
			 void *end, const char *function, unsigned int line)
{
	return check_xattrs(inode, NULL, IFIRST(header), end, IFIRST(header),
			    function, line);
}

static int
xattr_find_entry(struct inode *inode, struct ext4_xattr_entry **pentry,
		 void *end, int name_index, const char *name, int sorted)
{
	struct ext4_xattr_entry *entry, *next;
	size_t name_len;
	int cmp = 1;

	if (name == NULL)
		return -EINVAL;
	name_len = strlen(name);
	for (entry = *pentry; !IS_LAST_ENTRY(entry); entry = next) {
		next = EXT4_XATTR_NEXT(entry);
		if ((char *) next >= (char *)end) {
			EXT4_ERROR_INODE(inode, "corrupted xattr entries");
			return -EFSCORRUPTED;
		}
		cmp = name_index - entry->e_name_index;
		if (!cmp)
			cmp = name_len - entry->e_name_len;
		if (!cmp)
			cmp = memcmp(name, entry->e_name, name_len);
		if (!cmp || (cmp < 0 && sorted))
			break;
	}
	*pentry = entry;
	return cmp ? -ENODATA : 0;
}

int ext4_xattr_ibody_find(struct inode *inode, struct ext4_xattr_info *i,
			  struct ext4_xattr_ibody_find *is)
{
	struct ext4_xattr_ibody_header *header;
	struct ext4_inode *raw_inode;
	int error;

	if (!EXT4_INODE_HAS_XATTR_SPACE(inode))
		return 0;

	raw_inode = ext4_raw_inode(&is->iloc);
	header = IHDR(inode, raw_inode);
	is->s.base = is->s.first = IFIRST(header);
	is->s.here = is->s.first;
	is->s.end = ITAIL(inode, raw_inode);
	if (ext4_test_inode_state(inode, EXT4_STATE_XATTR)) {
		/* Find the named attribute. */
		error = xattr_find_entry(inode, &is->s.here, is->s.end,
					 i->name_index, i->name, 0);
		if (error && error != -ENODATA)
			return error;
		is->s.not_found = error;
	}
	return 0;
}
