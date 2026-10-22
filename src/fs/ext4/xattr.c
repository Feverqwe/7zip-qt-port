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
#include "xattrread.h"
#include "aclcompat.h"

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

/* Linux handler-map namespaces. Archive metadata reads do not apply host VFS
 * capability/LSM policy; private system.data/fscrypt contexts are not listed. */
static const char *ext4_xattr_prefix(unsigned index)
{
    static const char *const prefixes[] = {
        NULL, "user.", "system.posix_acl_access", "system.posix_acl_default",
        "trusted.", NULL, "security.", NULL, NULL, NULL, "gnu."
    };
    return index < ARRAY_SIZE(prefixes) ? prefixes[index] : NULL;
}

static int ext4_xattr_export_error(struct inode *inode, int error)
{
    const struct ext_xattr_services *services = EXT4_SB(inode->i_sb)->xattr_services;
    if (error == -ENOMEM || !services || !services->handle_error) return error;
    return services->handle_error(services->context, error);
}

static int ext4_xattr_list_entries(struct inode *inode, struct ext4_xattr_entry *entry,
                                    ext_xattr_emit emit, void *context)
{
    for (; !IS_LAST_ENTRY(entry); entry = EXT4_XATTR_NEXT(entry)) {
        const char *prefix = ext4_xattr_prefix(entry->e_name_index);
        char name[256], *full_name;
        size_t prefix_size, output_size;
        void *value, *output;
        int size, result;
        if (!prefix) continue;
        memcpy(name, entry->e_name, entry->e_name_len);
        name[entry->e_name_len] = 0;
        size = ext4_xattr_get(inode, entry->e_name_index, name, NULL, 0);
        if (size < 0) {
            result = ext4_xattr_export_error(inode, size);
            if (result) return result;
            continue;
        }
        value = malloc(size ? (size_t)size : 1);
        if (!value) return -ENOMEM;
        result = ext4_xattr_get(inode, entry->e_name_index, name, value, size);
        if (result != size) {
            free(value);
            result = ext4_xattr_export_error(inode, result < 0 ? result : -EIO);
            if (result) return result;
            continue;
        }
        output = value;
        output_size = size;
        if (entry->e_name_index == 2 || entry->e_name_index == 3) {
            result = ext4_acl_xattr_value(value, size, &output, &output_size);
            if (result) {
                free(value);
                if (result == -ENODATA) continue;
                result = ext4_xattr_export_error(inode, result);
                if (result) return result;
                continue;
            }
        }
        prefix_size = strlen(prefix);
        full_name = malloc(prefix_size + entry->e_name_len + 1);
        if (!full_name) result = -ENOMEM;
        else {
            memcpy(full_name, prefix, prefix_size);
            memcpy(full_name + prefix_size, name, entry->e_name_len + 1);
            result = emit(context, full_name, output, output_size);
            free(full_name);
        }
        if (output != value) free(output);
        free(value);
        if (result) return result;
    }
    return 0;
}

int ext4_xattr_visit(struct inode *inode, ext_xattr_emit emit, void *context)
{
    struct ext4_iloc iloc;
    struct buffer_head *bh;
    int result;
    if (ext4_test_inode_state(inode, EXT4_STATE_XATTR)) {
        ext4_get_inode_loc(inode, &iloc);
        result = ext4_xattr_list_entries(inode, IFIRST(IHDR(inode, ext4_raw_inode(&iloc))),
                                         emit, context);
        brelse(iloc.bh);
        if (result) return result;
    }
    if (!EXT4_I(inode)->i_file_acl) return 0;
    if (!ext4_inode_block_valid(inode, EXT4_I(inode)->i_file_acl, 1))
        return ext4_xattr_export_error(inode, -EFSCORRUPTED);
    bh = ext4_sb_bread(inode->i_sb, EXT4_I(inode)->i_file_acl, REQ_PRIO);
    if (IS_ERR(bh)) return ext4_xattr_export_error(inode, PTR_ERR(bh));
    result = ext4_xattr_check_block(inode, bh);
    if (result) result = ext4_xattr_export_error(inode, result);
    else result = ext4_xattr_list_entries(inode, BFIRST(bh), emit, context);
    brelse(bh);
    return result;
}

/* Read getters transferred from the pinned Linux xattr implementation. */

static __le32 ext4_xattr_hash_entry(char *, size_t, __le32 *, size_t);
static __le32 ext4_xattr_hash_entry_signed(char *, size_t, __le32 *, size_t);

static u32
ext4_xattr_inode_hash(struct ext4_sb_info *sbi, const void *buffer, size_t size)
{
	return ext4_chksum(sbi->s_csum_seed, buffer, size);
}

static u64 ext4_xattr_inode_get_ref(struct inode *ea_inode)
{
	return ((u64) inode_get_ctime_sec(ea_inode) << 32) |
		(u32) inode_peek_iversion_raw(ea_inode);
}

static void ext4_xattr_inode_set_ref(struct inode *ea_inode, u64 ref_count)
{
	inode_set_ctime(ea_inode, (u32)(ref_count >> 32), 0);
	inode_set_iversion_raw(ea_inode, ref_count & 0xffffffff);
}

static u32 ext4_xattr_inode_get_hash(struct inode *ea_inode)
{
	return (u32) inode_get_atime_sec(ea_inode);
}


static int ext4_xattr_inode_read(struct inode *ea_inode, void *buf, size_t size)
{
	int blocksize = 1 << ea_inode->i_blkbits;
	int bh_count = (size + blocksize - 1) >> ea_inode->i_blkbits;
	int tail_size = size % blocksize ? (int)(size % blocksize) : blocksize;
	struct buffer_head *bhs_inline[8];
	struct buffer_head **bhs = bhs_inline;
	int i, ret;

	if (bh_count > ARRAY_SIZE(bhs_inline)) {
		bhs = kmalloc_objs(*bhs, bh_count, GFP_NOFS);
		if (!bhs)
			return -ENOMEM;
	}

	ret = ext4_bread_batch(ea_inode, 0 /* block */, bh_count,
			       true /* wait */, bhs);
	if (ret)
		goto free_bhs;

	for (i = 0; i < bh_count; i++) {
		/* There shouldn't be any holes in ea_inode. */
		if (!bhs[i]) {
			ret = -EFSCORRUPTED;
			goto put_bhs;
		}
		memcpy((char *)buf + blocksize * i, bhs[i]->b_data,
		       i < bh_count - 1 ? blocksize : tail_size);
	}
	ret = 0;
put_bhs:
	for (i = 0; i < bh_count; i++)
		brelse(bhs[i]);
free_bhs:
	if (bhs != bhs_inline)
		kfree(bhs);
	return ret;
}

#define EXT4_XATTR_INODE_GET_PARENT(inode) ((__u32)(inode_get_mtime_sec(inode)))

static int ext4_xattr_inode_iget(struct inode *parent, unsigned long ea_ino,
				 u32 ea_inode_hash, struct inode **ea_inode)
{
	struct inode *inode;
	int err;

	/*
	 * We have to check for this corruption early as otherwise
	 * iget_locked() could wait indefinitely for the state of our
	 * parent inode.
	 */
	if (parent->i_ino == ea_ino) {
		ext4_error(parent->i_sb,
			   "Parent and EA inode have the same ino %lu", ea_ino);
		return -EFSCORRUPTED;
	}

	inode = ext4_iget(parent->i_sb, ea_ino, EXT4_IGET_EA_INODE);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		ext4_error(parent->i_sb,
			   "error while reading EA inode %lu err=%d", ea_ino,
			   err);
		return err;
	}
	ext4_xattr_inode_set_class(inode);

	/*
	 * Check whether this is an old Lustre-style xattr inode. Lustre
	 * implementation does not have hash validation, rather it has a
	 * backpointer from ea_inode to the parent inode.
	 */
	if (ea_inode_hash != ext4_xattr_inode_get_hash(inode) &&
	    EXT4_XATTR_INODE_GET_PARENT(inode) == parent->i_ino &&
	    inode->i_generation == parent->i_generation) {
		ext4_set_inode_state(inode, EXT4_STATE_LUSTRE_EA_INODE);
		ext4_xattr_inode_set_ref(inode, 1);
	} else {
		inode_lock_nested(inode, I_MUTEX_XATTR);
		inode->i_flags |= S_NOQUOTA;
		inode_unlock(inode);
	}

	/*
	 * Since this function resolves references from active xattr entries,
	 * the EA inode must be in active state (i_nlink=1, ref_count>0).
	 * i_nlink > 1, i_nlink == 0 (dangling reference), or ref_count == 0
	 * (inconsistent with an active entry) all indicate on-disk corruption.
	 */
	if (inode->i_nlink != 1 || !ext4_xattr_inode_get_ref(inode)) {
		ext4_error(parent->i_sb,
			   "EA inode %lu has unexpected i_nlink=%u ref_count=%llu",
			   ea_ino, inode->i_nlink,
			   ext4_xattr_inode_get_ref(inode));
		ext4_put_ea_inode(inode);
		return -EFSCORRUPTED;
	}

	*ea_inode = inode;
	return 0;
}


static int
ext4_xattr_inode_verify_hashes(struct inode *ea_inode,
			       struct ext4_xattr_entry *entry, void *buffer,
			       size_t size)
{
	u32 hash;

	/* Verify stored hash matches calculated hash. */
	hash = ext4_xattr_inode_hash(EXT4_SB(ea_inode->i_sb), buffer, size);
	if (hash != ext4_xattr_inode_get_hash(ea_inode))
		return -EFSCORRUPTED;

	if (entry) {
		__le32 e_hash, tmp_data;

		/* Verify entry hash. */
		tmp_data = cpu_to_le32(hash);
		e_hash = ext4_xattr_hash_entry(entry->e_name, entry->e_name_len,
					       &tmp_data, 1);
		/* All good? */
		if (e_hash == entry->e_hash)
			return 0;

		/*
		 * Not good. Maybe the entry hash was calculated
		 * using the buggy signed char version?
		 */
		e_hash = ext4_xattr_hash_entry_signed(entry->e_name, entry->e_name_len,
							&tmp_data, 1);
		/* Still no match - bad */
		if (e_hash != entry->e_hash)
			return -EFSCORRUPTED;

		/* Let people know about old hash */
		pr_warn_once("ext4: filesystem with signed xattr name hash");
	}
	return 0;
}

/*
 * Read xattr value from the EA inode.
 */
static int
ext4_xattr_inode_get(struct inode *inode, struct ext4_xattr_entry *entry,
		     void *buffer, size_t size)
{
	struct mb_cache *ea_inode_cache = EA_INODE_CACHE(inode);
	struct inode *ea_inode;
	int err;

	err = ext4_xattr_inode_iget(inode, le32_to_cpu(entry->e_value_inum),
				    le32_to_cpu(entry->e_hash), &ea_inode);
	if (err) {
		ea_inode = NULL;
		goto out;
	}

	if (i_size_read(ea_inode) != size) {
		ext4_warning_inode(ea_inode,
				   "ea_inode file size=%llu entry size=%zu",
				   i_size_read(ea_inode), size);
		err = -EFSCORRUPTED;
		goto out;
	}

	err = ext4_xattr_inode_read(ea_inode, buffer, size);
	if (err)
		goto out;

	if (!ext4_test_inode_state(ea_inode, EXT4_STATE_LUSTRE_EA_INODE)) {
		err = ext4_xattr_inode_verify_hashes(ea_inode, entry, buffer,
						     size);
		if (err) {
			ext4_warning_inode(ea_inode,
					   "EA inode hash validation failed");
			goto out;
		}

		if (ea_inode_cache)
			mb_cache_entry_create(ea_inode_cache, GFP_NOFS,
					ext4_xattr_inode_get_hash(ea_inode),
					ea_inode->i_ino, true /* reusable */);
	}
out:
	ext4_put_ea_inode(ea_inode);
	return err;
}

static int
ext4_xattr_block_get(struct inode *inode, int name_index, const char *name,
		     void *buffer, size_t buffer_size)
{
	struct buffer_head *bh = NULL;
	struct ext4_xattr_entry *entry;
	size_t size;
	void *end;
	int error;
	struct mb_cache *ea_block_cache = EA_BLOCK_CACHE(inode);

	ea_idebug(inode, "name=%d.%s, buffer=%p, buffer_size=%ld",
		  name_index, name, buffer, (long)buffer_size);

	if (!EXT4_I(inode)->i_file_acl)
		return -ENODATA;
	/* The eager archive inode loader defers optional xattr validation here. */
	if (!ext4_inode_block_valid(inode, EXT4_I(inode)->i_file_acl, 1))
		return -EFSCORRUPTED;
	ea_idebug(inode, "reading block %llu",
		  (unsigned long long)EXT4_I(inode)->i_file_acl);
	bh = ext4_sb_bread(inode->i_sb, EXT4_I(inode)->i_file_acl, REQ_PRIO);
	if (IS_ERR(bh))
		return PTR_ERR(bh);
	ea_bdebug(bh, "b_count=%d, refcount=%d",
		atomic_read(&(bh->b_count)), le32_to_cpu(BHDR(bh)->h_refcount));
	error = ext4_xattr_check_block(inode, bh);
	if (error)
		goto cleanup;
	ext4_xattr_block_cache_insert(ea_block_cache, bh);
	entry = BFIRST(bh);
	end = bh->b_data + bh->b_size;
	error = xattr_find_entry(inode, &entry, end, name_index, name, 1);
	if (error)
		goto cleanup;
	size = le32_to_cpu(entry->e_value_size);
	error = -ERANGE;
	if (unlikely(size > EXT4_XATTR_SIZE_MAX))
		goto cleanup;
	if (buffer) {
		if (size > buffer_size)
			goto cleanup;
		if (entry->e_value_inum) {
			error = ext4_xattr_inode_get(inode, entry, buffer,
						     size);
			if (error)
				goto cleanup;
		} else {
			u16 offset = le16_to_cpu(entry->e_value_offs);
			void *p = bh->b_data + offset;

			if (unlikely((char *)p + size > (char *)end))
				goto cleanup;
			memcpy(buffer, p, size);
		}
	}
	error = size;

cleanup:
	brelse(bh);
	return error;
}

int
ext4_xattr_ibody_get(struct inode *inode, int name_index, const char *name,
		     void *buffer, size_t buffer_size)
{
	struct ext4_xattr_ibody_header *header;
	struct ext4_xattr_entry *entry;
	struct ext4_inode *raw_inode;
	struct ext4_iloc iloc;
	size_t size;
	void *end;
	int error;

	if (!ext4_test_inode_state(inode, EXT4_STATE_XATTR))
		return -ENODATA;
	error = ext4_get_inode_loc(inode, &iloc);
	if (error)
		return error;
	raw_inode = ext4_raw_inode(&iloc);
	header = IHDR(inode, raw_inode);
	end = ITAIL(inode, raw_inode);
	entry = IFIRST(header);
	error = xattr_find_entry(inode, &entry, end, name_index, name, 0);
	if (error)
		goto cleanup;
	size = le32_to_cpu(entry->e_value_size);
	error = -ERANGE;
	if (unlikely(size > EXT4_XATTR_SIZE_MAX))
		goto cleanup;
	if (buffer) {
		if (size > buffer_size)
			goto cleanup;
		if (entry->e_value_inum) {
			error = ext4_xattr_inode_get(inode, entry, buffer,
						     size);
			if (error)
				goto cleanup;
		} else {
			u16 offset = le16_to_cpu(entry->e_value_offs);
			void *p = (char *)IFIRST(header) + offset;

			if (unlikely((char *)p + size > (char *)end))
				goto cleanup;
			memcpy(buffer, p, size);
		}
	}
	error = size;

cleanup:
	brelse(iloc.bh);
	return error;
}

/*
 * ext4_xattr_get()
 *
 * Copy an extended attribute into the buffer
 * provided, or compute the buffer size required.
 * Buffer is NULL to compute the size of the buffer required.
 *
 * Returns a negative error number on failure, or the number of bytes
 * used / required on success.
 */
int
ext4_xattr_get(struct inode *inode, int name_index, const char *name,
	       void *buffer, size_t buffer_size)
{
	int error;

	if (unlikely(ext4_forced_shutdown(inode->i_sb)))
		return -EIO;

	if (strlen(name) > 255)
		return -ERANGE;

	down_read(&EXT4_I(inode)->xattr_sem);
	error = ext4_xattr_ibody_get(inode, name_index, name, buffer,
				     buffer_size);
	if (error == -ENODATA)
		error = ext4_xattr_block_get(inode, name_index, name, buffer,
					     buffer_size);
	up_read(&EXT4_I(inode)->xattr_sem);
	return error;
}


#define NAME_HASH_SHIFT 5
#define VALUE_HASH_SHIFT 16

/*
 * ext4_xattr_hash_entry()
 *
 * Compute the hash of an extended attribute.
 */
static __le32 ext4_xattr_hash_entry(char *name, size_t name_len, __le32 *value,
				    size_t value_count)
{
	__u32 hash = 0;

	while (name_len--) {
		hash = (hash << NAME_HASH_SHIFT) ^
		       (hash >> (8*sizeof(hash) - NAME_HASH_SHIFT)) ^
		       (unsigned char)*name++;
	}
	while (value_count--) {
		hash = (hash << VALUE_HASH_SHIFT) ^
		       (hash >> (8*sizeof(hash) - VALUE_HASH_SHIFT)) ^
		       le32_to_cpu(*value++);
	}
	return cpu_to_le32(hash);
}

/*
 * ext4_xattr_hash_entry_signed()
 *
 * Compute the hash of an extended attribute incorrectly.
 */
static __le32 ext4_xattr_hash_entry_signed(char *name, size_t name_len, __le32 *value, size_t value_count)
{
	__u32 hash = 0;

	while (name_len--) {
		hash = (hash << NAME_HASH_SHIFT) ^
		       (hash >> (8*sizeof(hash) - NAME_HASH_SHIFT)) ^
		       (signed char)*name++;
	}
	while (value_count--) {
		hash = (hash << VALUE_HASH_SHIFT) ^
		       (hash >> (8*sizeof(hash) - VALUE_HASH_SHIFT)) ^
		       le32_to_cpu(*value++);
	}
	return cpu_to_le32(hash);
}

#undef NAME_HASH_SHIFT
#undef VALUE_HASH_SHIFT
