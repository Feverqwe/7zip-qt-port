// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/ext4/namei.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 *  Directory entry file type support and forward compatibility hooks
 *	for B-tree directories by Theodore Ts'o (tytso@mit.edu), 1998
 *  Hash Tree Directory indexing (c)
 *	Daniel Phillips, 2001
 *  Hash Tree Directory indexing porting
 *	Christopher Li, 2002
 *  Hash Tree Directory indexing cleanup
 *	Theodore Ts'o, 2002
 */

#include "formats/ext/ext4.h"
struct dx_entry
{
	__le32 hash;
	__le32 block;
};

struct dx_countlimit
{
	__le16 limit;
	__le16 count;
};

struct fake_dirent
{
	__le32 inode;
	__le16 rec_len;
	u8 name_len;
	u8 file_type;
};

struct dx_root
{
	struct fake_dirent dot;
	char dot_name[4];
	struct fake_dirent dotdot;
	char dotdot_name[4];
	struct dx_root_info
	{
		__le32 reserved_zero;
		u8 hash_version;
		u8 info_length; /* 8 */
		u8 indirect_levels;
		u8 unused_flags;
	}
	info;
	struct dx_entry	entries[];
};

struct dx_tail {
	u32 dt_reserved;
	__le32 dt_checksum;	/* crc32c(uuid+inum+dirblock) */
};

#define warn_no_space_for_csum(inode) __warn_no_space_for_csum((inode), __func__, __LINE__)

/* Walk through a dirent block to find a checksum "dirent" at the tail */
static struct ext4_dir_entry_tail *get_dirent_tail(struct inode *inode,
						   struct buffer_head *bh)
{
	struct ext4_dir_entry_tail *t;
	int blocksize = EXT4_BLOCK_SIZE(inode->i_sb);

#ifdef PARANOID
	struct ext4_dir_entry *d, *top;

	d = (struct ext4_dir_entry *)bh->b_data;
	top = (struct ext4_dir_entry *)(bh->b_data +
		(blocksize - sizeof(struct ext4_dir_entry_tail)));
	while (d < top && ext4_rec_len_from_disk(d->rec_len, blocksize))
		d = (struct ext4_dir_entry *)(((char *)d) +
		    ext4_rec_len_from_disk(d->rec_len, blocksize));

	if (d != top)
		return NULL;

	t = (struct ext4_dir_entry_tail *)d;
#else
	t = EXT4_DIRENT_TAIL(bh->b_data, EXT4_BLOCK_SIZE(inode->i_sb));
#endif

	if (t->det_reserved_zero1 ||
	    (ext4_rec_len_from_disk(t->det_rec_len, blocksize) !=
	     sizeof(struct ext4_dir_entry_tail)) ||
	    t->det_reserved_zero2 ||
	    t->det_reserved_ft != EXT4_FT_DIR_CSUM)
		return NULL;

	return t;
}

static __le32 ext4_dirblock_csum(struct inode *inode, void *dirent, int size)
{
	struct ext4_inode_info *ei = EXT4_I(inode);
	__u32 csum;

	csum = ext4_chksum(ei->i_csum_seed, (__u8 *)dirent, size);
	return cpu_to_le32(csum);
}

#define warn_no_space_for_csum(inode)					\
	__warn_no_space_for_csum((inode), __func__, __LINE__)

static void __warn_no_space_for_csum(struct inode *inode, const char *func,
				     unsigned int line)
{
	__ext4_warning_inode(inode, func, line,
		"No space for directory leaf checksum. Please run e2fsck -D.");
}

int ext4_dirblock_csum_verify(struct inode *inode, struct buffer_head *bh)
{
	struct ext4_dir_entry_tail *t;

	if (!ext4_has_feature_metadata_csum(inode->i_sb))
		return 1;

	t = get_dirent_tail(inode, bh);
	if (!t) {
		warn_no_space_for_csum(inode);
		return 0;
	}

	if (t->det_checksum != ext4_dirblock_csum(inode, bh->b_data,
						  (char *)t - bh->b_data))
		return 0;

	return 1;
}

static struct dx_countlimit *get_dx_countlimit(struct inode *inode,
					       struct ext4_dir_entry *dirent,
					       int *offset)
{
	struct ext4_dir_entry *dp;
	struct dx_root_info *root;
	int count_offset;
	int blocksize = EXT4_BLOCK_SIZE(inode->i_sb);
	unsigned int rlen = ext4_rec_len_from_disk(dirent->rec_len, blocksize);

	if (rlen == blocksize)
		count_offset = 8;
	else if (rlen == 12) {
		dp = (struct ext4_dir_entry *)(((char *)dirent) + 12);
		if (ext4_rec_len_from_disk(dp->rec_len, blocksize) != blocksize - 12)
			return NULL;
		root = (struct dx_root_info *)(((char *)dp + 12));
		if (root->reserved_zero ||
		    root->info_length != sizeof(struct dx_root_info))
			return NULL;
		count_offset = 32;
	} else
		return NULL;

	if (offset)
		*offset = count_offset;
	return (struct dx_countlimit *)(((char *)dirent) + count_offset);
}

static __le32 ext4_dx_csum(struct inode *inode, struct ext4_dir_entry *dirent,
			   int count_offset, int count, struct dx_tail *t)
{
	struct ext4_inode_info *ei = EXT4_I(inode);
	__u32 csum;
	int size;
	__u32 dummy_csum = 0;
	int offset = offsetof(struct dx_tail, dt_checksum);

	size = count_offset + (count * sizeof(struct dx_entry));
	csum = ext4_chksum(ei->i_csum_seed, (__u8 *)dirent, size);
	csum = ext4_chksum(csum, (__u8 *)t, offset);
	csum = ext4_chksum(csum, (__u8 *)&dummy_csum, sizeof(dummy_csum));

	return cpu_to_le32(csum);
}

int ext4_dx_csum_verify(struct inode *inode,
			       struct ext4_dir_entry *dirent)
{
	struct dx_countlimit *c;
	struct dx_tail *t;
	int count_offset, limit, count;

	if (!ext4_has_feature_metadata_csum(inode->i_sb))
		return 1;

	c = get_dx_countlimit(inode, dirent, &count_offset);
	if (!c) {
		EXT4_ERROR_INODE(inode, "dir seems corrupt?  Run e2fsck -D.");
		return 0;
	}
	limit = le16_to_cpu(c->limit);
	count = le16_to_cpu(c->count);
	/* The VFS htree parser normally validates this before checksum lookup. */
	if (count == 0 || count > limit)
		return 0;
	if (count_offset + (limit * sizeof(struct dx_entry)) >
	    EXT4_BLOCK_SIZE(inode->i_sb) - sizeof(struct dx_tail)) {
		warn_no_space_for_csum(inode);
		return 0;
	}
	t = (struct dx_tail *)(((struct dx_entry *)c) + limit);

	if (t->dt_checksum != ext4_dx_csum(inode, dirent, count_offset,
					    count, t))
		return 0;
	return 1;
}

/* Archive htree read services: immutable logical blocks, explicit ownership. */
#include "formats/ext/hashread.h"
#include "formats/ext/blockread.h"
#include "formats/ext/extreaderprivate.h"
#define dxtrace(command) do {} while (0)
#define htree_rep_invariant_check(at, target, hash, count) ((void)0)
#define ext4_simulate_fail(sb, type) 0
#define __ext4_warning(...) ((void)0)
#define ext4_error(sb, ...) ((void)0)
struct dx_node { struct fake_dirent fake; struct dx_entry entries[]; };
struct dx_frame { struct buffer_head *bh; struct dx_entry *entries, *at; };
typedef enum { EITHER, INDEX, DIRENT, DIRENT_HTREE } dirblock_type_t;
struct ext_index_inode {
    struct inode inode;
    void *context;
    ext_dir_read_block read;
    unsigned char *visited;
    u64 visited_bits;
};
static struct buffer_head *ext4_bread(void *handle, struct inode *inode, ext4_lblk_t block, int flags)
{
    struct ext_index_inode *owner = (struct ext_index_inode *)inode;
    struct buffer_head *bh;
    int error;
    (void)handle; (void)flags;
    if (block >= owner->visited_bits || (owner->visited[block / 8] & (1U << (block % 8))))
        return ERR_PTR(-EFSCORRUPTED);
    owner->visited[block / 8] |= 1U << (block % 8);
    bh = calloc(1, sizeof(*bh) + inode->i_sb->s_blocksize);
    if (!bh) return ERR_PTR(-ENOMEM);
    bh->b_data = (char *)(bh + 1);
    bh->b_size = inode->i_sb->s_blocksize;
    bh->allocated = true;
    error = owner->read(owner->context, block, bh->b_data, bh->b_size);
    if (error) { brelse(bh); return error == 1 ? NULL : ERR_PTR(error); }
    return bh;
}

static inline ext4_lblk_t dx_get_block(struct dx_entry *entry)
{
	return le32_to_cpu(entry->block) & 0x0fffffff;
}

static inline unsigned dx_get_hash(struct dx_entry *entry)
{
	return le32_to_cpu(entry->hash);
}

static inline unsigned dx_get_count(struct dx_entry *entries)
{
	return le16_to_cpu(((struct dx_countlimit *) entries)->count);
}

static inline unsigned dx_get_limit(struct dx_entry *entries)
{
	return le16_to_cpu(((struct dx_countlimit *) entries)->limit);
}

static inline unsigned dx_root_limit(struct inode *dir, unsigned infosize)
{
	unsigned int entry_space = dir->i_sb->s_blocksize -
			ext4_dir_rec_len(1, NULL) -
			ext4_dir_rec_len(2, NULL) - infosize;

	if (ext4_has_feature_metadata_csum(dir->i_sb))
		entry_space -= sizeof(struct dx_tail);
	return entry_space / sizeof(struct dx_entry);
}

static inline unsigned dx_node_limit(struct inode *dir)
{
	unsigned int entry_space = dir->i_sb->s_blocksize -
			ext4_dir_rec_len(0, dir);

	if (ext4_has_feature_metadata_csum(dir->i_sb))
		entry_space -= sizeof(struct dx_tail);
	return entry_space / sizeof(struct dx_entry);
}

static struct buffer_head *__ext4_read_dirblock(struct inode *inode,
						ext4_lblk_t block,
						dirblock_type_t type,
						const char *func,
						unsigned int line)
{
	struct buffer_head *bh;
	struct ext4_dir_entry *dirent;
	int is_dx_block = 0;

	if (block >= inode->i_size >> inode->i_blkbits) {
		ext4_error_inode(inode, func, line, block,
		       "Attempting to read directory block (%u) that is past i_size (%llu)",
		       block, inode->i_size);
		return ERR_PTR(-EFSCORRUPTED);
	}

	if (ext4_simulate_fail(inode->i_sb, EXT4_SIM_DIRBLOCK_EIO))
		bh = ERR_PTR(-EIO);
	else
		bh = ext4_bread(NULL, inode, block, 0);
	if (IS_ERR(bh)) {
		__ext4_warning(inode->i_sb, func, line,
			       "inode #%llu: lblock %lu: comm %s: "
			       "error %pe reading directory block",
			       inode->i_ino, (unsigned long)block,
			       current->comm, bh);

		return bh;
	}
	/* The first directory block must not be a hole. */
	if (!bh && (type == INDEX || type == DIRENT_HTREE || block == 0)) {
		ext4_error_inode(inode, func, line, block,
				 "Directory hole found for htree %s block %u",
				 (type == INDEX) ? "index" : "leaf", block);
		return ERR_PTR(-EFSCORRUPTED);
	}
	if (!bh)
		return NULL;
	dirent = (struct ext4_dir_entry *) bh->b_data;
	/* Determine whether or not we have an index block */
	if (is_dx(inode)) {
		if (block == 0)
			is_dx_block = 1;
		else if (ext4_rec_len_from_disk(dirent->rec_len,
						inode->i_sb->s_blocksize) ==
			 inode->i_sb->s_blocksize)
			is_dx_block = 1;
	}
	if (!is_dx_block && type == INDEX) {
		ext4_error_inode(inode, func, line, block,
		       "directory leaf block found instead of index block");
		brelse(bh);
		return ERR_PTR(-EFSCORRUPTED);
	}
	if (!ext4_has_feature_metadata_csum(inode->i_sb) ||
	    buffer_verified(bh))
		return bh;

	/*
	 * An empty leaf block can get mistaken for a index block; for
	 * this reason, we can only check the index checksum when the
	 * caller is sure it should be an index block.
	 */
	if (is_dx_block && type == INDEX) {
		if (ext4_dx_csum_verify(inode, dirent) &&
		    !ext4_simulate_fail(inode->i_sb, EXT4_SIM_DIRBLOCK_CRC))
			set_buffer_verified(bh);
		else {
			ext4_error_inode_err(inode, func, line, block,
					     EFSBADCRC,
					     "Directory index failed checksum");
			brelse(bh);
			return ERR_PTR(-EFSBADCRC);
		}
	}
	if (!is_dx_block) {
		if (ext4_dirblock_csum_verify(inode, bh) &&
		    !ext4_simulate_fail(inode->i_sb, EXT4_SIM_DIRBLOCK_CRC))
			set_buffer_verified(bh);
		else {
			ext4_error_inode_err(inode, func, line, block,
					     EFSBADCRC,
					     "Directory block failed checksum");
			brelse(bh);
			return ERR_PTR(-EFSBADCRC);
		}
	}
	return bh;
}


/* Preserve index-reader preconditions even without metadata checksums. */
static struct buffer_head *ext4_read_dirblock(struct inode *inode, ext4_lblk_t block, dirblock_type_t type)
{
    struct buffer_head *bh = __ext4_read_dirblock(inode, block, type, __func__, __LINE__);
    struct dx_countlimit *count;
    unsigned limit;
    if (IS_ERR(bh) || !bh) return bh;
    if (type == INDEX) {
        unsigned offset = sizeof(struct fake_dirent);
        if (block == 0) {
            unsigned infosize = ((struct dx_root *)bh->b_data)->info.info_length;
            if (infosize < sizeof(struct dx_root_info) || (infosize & 3))
                goto bad_index;
            offset = offsetof(struct dx_root, info) + infosize;
            limit = dx_root_limit(inode, infosize);
        } else {
            limit = dx_node_limit(inode);
        }
        if (offset + sizeof(struct dx_entry) > bh->b_size)
            goto bad_index;
        count = (struct dx_countlimit *)(bh->b_data + offset);
        if (le16_to_cpu(count->limit) != limit ||
            !le16_to_cpu(count->count) || le16_to_cpu(count->count) > limit)
            goto bad_index;
    } else if (!ext4_dirblock_csum_verify(inode, bh)) {
        goto corrupt;
    }
    return bh;
corrupt:
    brelse(bh);
    return ERR_PTR(-EFSCORRUPTED);
bad_index:
    brelse(bh);
    return ERR_PTR(ERR_BAD_DX_DIR);
}

static struct dx_frame *
dx_probe(struct inode *dir,
	 struct dx_hash_info *hinfo, struct dx_frame *frame_in)
{
	unsigned count, indirect, level, i;
	struct dx_entry *at, *entries, *p, *q, *m;
	struct dx_root *root;
	struct dx_frame *frame = frame_in;
	struct dx_frame *ret_err = ERR_PTR(ERR_BAD_DX_DIR);
	u32 hash;
	ext4_lblk_t block;
	ext4_lblk_t blocks[EXT4_HTREE_LEVEL];

	memset(frame_in, 0, EXT4_HTREE_LEVEL * sizeof(frame_in[0]));
	frame->bh = ext4_read_dirblock(dir, 0, INDEX);
	if (IS_ERR(frame->bh))
		return (struct dx_frame *) frame->bh;

	root = (struct dx_root *) frame->bh->b_data;
	if (root->info.hash_version != DX_HASH_TEA &&
	    root->info.hash_version != DX_HASH_HALF_MD4 &&
	    root->info.hash_version != DX_HASH_LEGACY &&
	    root->info.hash_version != DX_HASH_SIPHASH) {
		ext4_warning_inode(dir, "Unrecognised inode hash code %u",
				   root->info.hash_version);
		goto fail;
	}
	if (ext4_hash_in_dirent(dir)) {
		if (root->info.hash_version != DX_HASH_SIPHASH) {
			ext4_warning_inode(dir,
				"Hash in dirent, but hash is not SIPHASH");
			goto fail;
		}
	} else {
		if (root->info.hash_version == DX_HASH_SIPHASH) {
			ext4_warning_inode(dir,
				"Hash code is SIPHASH, but hash not in dirent");
			goto fail;
		}
	}
	hinfo->hash_version = root->info.hash_version;
	if (hinfo->hash_version <= DX_HASH_TEA)
		hinfo->hash_version += EXT4_SB(dir->i_sb)->s_hash_unsigned;
	hinfo->seed = EXT4_SB(dir->i_sb)->s_hash_seed;
	hash = hinfo->hash;

	if (root->info.unused_flags & 1) {
		ext4_warning_inode(dir, "Unimplemented hash flags: %#06x",
				   root->info.unused_flags);
		goto fail;
	}

	indirect = root->info.indirect_levels;
	if (indirect >= ext4_dir_htree_level(dir->i_sb)) {
		ext4_warning(dir->i_sb,
			     "Directory (ino: %llu) htree depth %#06x exceed"
			     "supported value", dir->i_ino,
			     ext4_dir_htree_level(dir->i_sb));
		if (ext4_dir_htree_level(dir->i_sb) < EXT4_HTREE_LEVEL) {
			ext4_warning(dir->i_sb, "Enable large directory "
						"feature to access it");
		}
		goto fail;
	}

	entries = (struct dx_entry *)(((char *)&root->info) +
				      root->info.info_length);

	if (dx_get_limit(entries) != dx_root_limit(dir,
						   root->info.info_length)) {
		ext4_warning_inode(dir, "dx entry: limit %u != root limit %u",
				   dx_get_limit(entries),
				   dx_root_limit(dir, root->info.info_length));
		goto fail;
	}

	dxtrace(printk("Look up %x", hash));
	level = 0;
	blocks[0] = 0;
	while (1) {
		count = dx_get_count(entries);
		if (!count || count > dx_get_limit(entries)) {
			ext4_warning_inode(dir,
					   "dx entry: count %u beyond limit %u",
					   count, dx_get_limit(entries));
			goto fail;
		}

		p = entries + 1;
		q = entries + count - 1;
		while (p <= q) {
			m = p + (q - p) / 2;
			dxtrace(printk(KERN_CONT "."));
			if (dx_get_hash(m) > hash)
				q = m - 1;
			else
				p = m + 1;
		}

		htree_rep_invariant_check(entries, p, hash, count - 1);

		at = p - 1;
		dxtrace(printk(KERN_CONT " %x->%u\n",
			       at == entries ? 0 : dx_get_hash(at),
			       dx_get_block(at)));
		frame->entries = entries;
		frame->at = at;

		block = dx_get_block(at);
		for (i = 0; i <= level; i++) {
			if (blocks[i] == block) {
				ext4_warning_inode(dir,
					"dx entry: tree cycle block %u points back to block %u",
					blocks[level], block);
				goto fail;
			}
		}
		if (++level > indirect)
			return frame;
		blocks[level] = block;
		frame++;
		frame->bh = ext4_read_dirblock(dir, block, INDEX);
		if (IS_ERR(frame->bh)) {
			ret_err = (struct dx_frame *) frame->bh;
			frame->bh = NULL;
			goto fail;
		}

		entries = ((struct dx_node *) frame->bh->b_data)->entries;

		if (dx_get_limit(entries) != dx_node_limit(dir)) {
			ext4_warning_inode(dir,
				"dx entry: limit %u != node limit %u",
				dx_get_limit(entries), dx_node_limit(dir));
			goto fail;
		}
	}
fail:
	while (frame >= frame_in) {
		brelse(frame->bh);
		frame--;
	}

	if (ret_err == ERR_PTR(ERR_BAD_DX_DIR))
		ext4_warning_inode(dir,
			"Corrupt directory, running e2fsck is recommended");
	return ret_err;
}

static void dx_release(struct dx_frame *frames)
{
	struct dx_root_info *info;
	int i;
	unsigned int indirect_levels;

	if (frames[0].bh == NULL)
		return;

	info = &((struct dx_root *)frames[0].bh->b_data)->info;
	/* save local copy, "info" may be freed after brelse() */
	indirect_levels = info->indirect_levels;
	for (i = 0; i <= indirect_levels; i++) {
		if (frames[i].bh == NULL)
			break;
		brelse(frames[i].bh);
		frames[i].bh = NULL;
	}
}

static int ext4_htree_next_block(struct inode *dir, __u32 hash,
				 struct dx_frame *frame,
				 struct dx_frame *frames,
				 __u32 *start_hash)
{
	struct dx_frame *p;
	struct buffer_head *bh;
	int num_frames = 0;
	__u32 bhash;

	p = frame;
	/*
	 * Find the next leaf page by incrementing the frame pointer.
	 * If we run out of entries in the interior node, loop around and
	 * increment pointer in the parent node.  When we break out of
	 * this loop, num_frames indicates the number of interior
	 * nodes need to be read.
	 */
	while (1) {
		if (++(p->at) < p->entries + dx_get_count(p->entries))
			break;
		if (p == frames)
			return 0;
		num_frames++;
		p--;
	}

	/*
	 * If the hash is 1, then continue only if the next page has a
	 * continuation hash of any value.  This is used for readdir
	 * handling.  Otherwise, check to see if the hash matches the
	 * desired continuation hash.  If it doesn't, return since
	 * there's no point to read in the successive index pages.
	 */
	bhash = dx_get_hash(p->at);
	if (start_hash)
		*start_hash = bhash;
	if ((hash & 1) == 0) {
		if ((bhash & ~1) != hash)
			return 0;
	}
	/*
	 * If the hash is HASH_NB_ALWAYS, we always go to the next
	 * block so no check is necessary
	 */
	while (num_frames--) {
		bh = ext4_read_dirblock(dir, dx_get_block(p->at), INDEX);
		if (IS_ERR(bh))
			return PTR_ERR(bh);
		p++;
		brelse(p->bh);
		p->bh = bh;
		p->at = p->entries = ((struct dx_node *) bh->b_data)->entries;
	}
	return 1;
}


int ext_linux_index_directory(struct ext_volume *volume, uint32_t ino, uint32_t seed,
                              uint32_t flags, uint64_t size, void *context,
                              ext_dir_read_block read, ext_dir_parse_block emit)
{
    struct ext_index_inode owner = { .inode = { .i_sb = &volume->sb, .i_ino = ino,
        .flags = flags, .i_size = size, .i_blkbits = volume->sb.s_blocksize_bits,
        .info = { .i_csum_seed = seed } },
        .context = context, .read = read };
    struct dx_frame frames[EXT4_HTREE_LEVEL], *frame;
    struct dx_hash_info hinfo = { .hash = 0 };
    int error;
    owner.visited_bits = size >> volume->sb.s_blocksize_bits;
    if (owner.visited_bits > (1U << 28)) owner.visited_bits = 1U << 28;
    if (!owner.visited_bits) return -EFSCORRUPTED;
    owner.visited = calloc((size_t)((owner.visited_bits + 7) / 8), 1);
    if (!owner.visited) return -ENOMEM;
    frame = dx_probe(&owner.inode, &hinfo, frames);
    if (IS_ERR(frame)) {
        error = PTR_ERR(frame);
        free(owner.visited);
        return error == ERR_BAD_DX_DIR ? 1 : error;
    }
    error = emit(context, 0, frames[0].bh->b_data, volume->sb.s_blocksize);
    while (!error) {
        ext4_lblk_t block = dx_get_block(frame->at);
        struct buffer_head *bh = ext4_read_dirblock(&owner.inode, block, DIRENT_HTREE);
        if (IS_ERR(bh)) { error = PTR_ERR(bh); break; }
        if (!bh) { error = -EFSCORRUPTED; break; }
        error = emit(context, block, bh->b_data, bh->b_size);
        brelse(bh);
        if (error) break;
        error = ext4_htree_next_block(&owner.inode, HASH_NB_ALWAYS, frame, frames, NULL);
        if (error <= 0) break;
        error = 0;
    }
    dx_release(frames);
    free(owner.visited);
    return error;
}
