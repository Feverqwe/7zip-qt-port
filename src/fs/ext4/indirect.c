// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/ext4/indirect.c
 *
 *  from
 *
 *  linux/fs/ext4/inode.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Goal-directed block allocation by Stephen Tweedie
 *	(sct@redhat.com), 1993, 1998
 */

#include "indirectread.h"

typedef struct {
	__le32	*p;
	__le32	key;
	struct buffer_head *bh;
} Indirect;

static inline void add_chain(Indirect *p, struct buffer_head *bh, __le32 *v)
{
	p->key = *(p->p = v);
	p->bh = bh;
}

/**
 *	ext4_block_to_path - parse the block number into array of offsets
 *	@inode: inode in question (we are only interested in its superblock)
 *	@i_block: block number to be parsed
 *	@offsets: array to store the offsets in
 *	@boundary: set this non-zero if the referred-to block is likely to be
 *	       followed (on disk) by an indirect block.
 *
 *	To store the locations of file's data ext4 uses a data structure common
 *	for UNIX filesystems - tree of pointers anchored in the inode, with
 *	data blocks at leaves and indirect blocks in intermediate nodes.
 *	This function translates the block number into path in that tree -
 *	return value is the path length and @offsets[n] is the offset of
 *	pointer to (n+1)th node in the nth one. If @block is out of range
 *	(negative or too large) warning is printed and zero returned.
 *
 *	Note: function doesn't find node addresses, so no IO is needed. All
 *	we need to know is the capacity of indirect blocks (taken from the
 *	inode->i_sb).
 */

/*
 * Portability note: the last comparison (check that we fit into triple
 * indirect block) is spelled differently, because otherwise on an
 * architecture with 32-bit longs and 8Kb pages we might get into trouble
 * if our filesystem had 8Kb blocks. We might use long long, but that would
 * kill us on x86. Oh, well, at least the sign propagation does not matter -
 * i_block would have to be negative in the very beginning, so we would not
 * get there at all.
 */

static int ext4_block_to_path(struct inode *inode,
			      ext4_lblk_t i_block,
			      ext4_lblk_t offsets[4], int *boundary)
{
	int ptrs = EXT4_ADDR_PER_BLOCK(inode->i_sb);
	int ptrs_bits = EXT4_ADDR_PER_BLOCK_BITS(inode->i_sb);
	const long direct_blocks = EXT4_NDIR_BLOCKS,
		indirect_blocks = ptrs,
		double_blocks = (1 << (ptrs_bits * 2));
	int n = 0;
	int final = 0;

	if (i_block < direct_blocks) {
		offsets[n++] = i_block;
		final = direct_blocks;
	} else if ((i_block -= direct_blocks) < indirect_blocks) {
		offsets[n++] = EXT4_IND_BLOCK;
		offsets[n++] = i_block;
		final = ptrs;
	} else if ((i_block -= indirect_blocks) < double_blocks) {
		offsets[n++] = EXT4_DIND_BLOCK;
		offsets[n++] = i_block >> ptrs_bits;
		offsets[n++] = i_block & (ptrs - 1);
		final = ptrs;
	} else if (((i_block -= double_blocks) >> (ptrs_bits * 2)) < ptrs) {
		offsets[n++] = EXT4_TIND_BLOCK;
		offsets[n++] = i_block >> (ptrs_bits * 2);
		offsets[n++] = (i_block >> ptrs_bits) & (ptrs - 1);
		offsets[n++] = i_block & (ptrs - 1);
		final = ptrs;
	} else {
		ext4_warning(inode->i_sb, "block %lu > max in inode %llu",
			     i_block + direct_blocks +
			     indirect_blocks + double_blocks, inode->i_ino);
	}
	if (boundary)
		*boundary = final - 1 - (i_block & (ptrs - 1));
	return n;
}

/**
 *	ext4_get_branch - read the chain of indirect blocks leading to data
 *	@inode: inode in question
 *	@depth: depth of the chain (1 - direct pointer, etc.)
 *	@offsets: offsets of pointers in inode/indirect blocks
 *	@chain: place to store the result
 *	@err: here we store the error value
 *
 *	Function fills the array of triples <key, p, bh> and returns %NULL
 *	if everything went OK or the pointer to the last filled triple
 *	(incomplete one) otherwise. Upon the return chain[i].key contains
 *	the number of (i+1)-th block in the chain (as it is stored in memory,
 *	i.e. little-endian 32-bit), chain[i].p contains the address of that
 *	number (it points into struct inode for i==0 and into the bh->b_data
 *	for i>0) and chain[i].bh points to the buffer_head of i-th indirect
 *	block for i>0 and NULL for i==0. In other words, it holds the block
 *	numbers of the chain, addresses they were taken from (and where we can
 *	verify that chain did not change) and buffer_heads hosting these
 *	numbers.
 *
 *	Function stops when it stumbles upon zero pointer (absent block)
 *		(pointer to last triple returned, *@err == 0)
 *	or when it gets an IO error reading an indirect block
 *		(ditto, *@err == -EIO)
 *	or when it reads all @depth-1 indirect blocks successfully and finds
 *	the whole chain, all way to the data (returns %NULL, *err == 0).
 *
 *      Need to be called with
 *      down_read(&EXT4_I(inode)->i_data_sem)
 */
static Indirect *ext4_get_branch(struct inode *inode, int depth,
				 ext4_lblk_t  *offsets,
				 Indirect chain[4], int *err)
{
	struct super_block *sb = inode->i_sb;
	Indirect *p = chain;
	struct buffer_head *bh;
	unsigned int key;
	int ret = -EIO;

	*err = 0;
	/* i_data is not going away, no lock needed */
	add_chain(chain, NULL, EXT4_I(inode)->i_data + *offsets);
	if (!p->key)
		goto no_block;
	while (--depth) {
		key = le32_to_cpu(p->key);
		if (key > ext4_blocks_count(EXT4_SB(sb)->s_es)) {
			/* the block was out of range */
			ret = -EFSCORRUPTED;
			goto failure;
		}
		bh = sb_getblk(sb, key);
		if (unlikely(!bh)) {
			ret = -ENOMEM;
			goto failure;
		}

		if (!bh_uptodate_or_lock(bh)) {
			if (ext4_read_bh(bh, 0, NULL, false) < 0) {
				put_bh(bh);
				goto failure;
			}
			/* validate block references */
			if (ext4_check_indirect_blockref(inode, bh)) {
				put_bh(bh);
				goto failure;
			}
		}

		add_chain(++p, bh, (__le32 *)bh->b_data + *++offsets);
		/* Reader: end */
		if (!p->key)
			goto no_block;
	}
	return NULL;

failure:
	*err = ret;
no_block:
	return p;
}

int ext4_ind_map_blocks(handle_t *handle, struct inode *inode,
			struct ext4_map_blocks *map,
			int flags)
{
	int err = -EIO;
	ext4_lblk_t offsets[4];
	Indirect chain[4];
	Indirect *partial;
	int blocks_to_boundary = 0;
	int depth;
	u64 count = 0;
	ext4_fsblk_t first_block = 0;

	trace_ext4_ind_map_blocks_enter(inode, map->m_lblk, map->m_len, flags);
	ASSERT(!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS)));
	ASSERT(handle != NULL || (flags & EXT4_GET_BLOCKS_CREATE) == 0);
	depth = ext4_block_to_path(inode, map->m_lblk, offsets,
				   &blocks_to_boundary);

	if (depth == 0)
		goto out;

	partial = ext4_get_branch(inode, depth, offsets, chain, &err);

	/* Simplest case - block found, no allocation needed */
	if (!partial) {
		first_block = le32_to_cpu(chain[depth - 1].key);
		count++;
		/*map more blocks*/
		while (count < map->m_len && count <= blocks_to_boundary) {
			ext4_fsblk_t blk;

			blk = le32_to_cpu(*(chain[depth-1].p + count));

			if (blk == first_block + count)
				count++;
			else
				break;
		}
		goto got_it;
	}

	/* Next simple case - plain lookup failed */
	if ((flags & EXT4_GET_BLOCKS_CREATE) == 0) {
		unsigned epb = inode->i_sb->s_blocksize / sizeof(u32);
		int i;

		/*
		 * Count number blocks in a subtree under 'partial'. At each
		 * level we count number of complete empty subtrees beyond
		 * current offset and then descend into the subtree only
		 * partially beyond current offset.
		 */
		count = 0;
		for (i = partial - chain + 1; i < depth; i++)
			count = count * epb + (epb - offsets[i] - 1);
		count++;
		/* Fill in size of a hole we found */
		map->m_pblk = 0;
		map->m_len = umin(map->m_len, count);
		goto cleanup;
	}

	/* Allocation/journal writes have no read-only archive caller. */
	err = -EOPNOTSUPP;
	goto cleanup;

got_it:
	map->m_flags |= EXT4_MAP_MAPPED;
	map->m_pblk = le32_to_cpu(chain[depth-1].key);
	map->m_len = count;
	if (count > blocks_to_boundary)
		map->m_flags |= EXT4_MAP_BOUNDARY;
	err = count;
	/* Clean up and exit */
	partial = chain + depth - 1;	/* the whole chain */
cleanup:
	while (partial > chain) {
		BUFFER_TRACE(partial->bh, "call brelse");
		brelse(partial->bh);
		partial--;
	}
out:
	trace_ext4_ind_map_blocks_exit(inode, flags, map, err);
	return err;
}


int ext_linux_indirect_open(struct ext_volume *volume, uint32_t ino, const void *refs,
                            const struct ext_indirect_services *services,
                            struct ext_indirect **output)
{
    struct ext_indirect *owner;
    int result;
    *output = NULL;
    if (!services || !services->read) return -EINVAL;
    owner = calloc(1, sizeof(*owner));
    if (!owner) return -ENOMEM;
    owner->sb = volume->sb;
    owner->services = *services;
    owner->inode.i_sb = &owner->sb;
    owner->inode.i_ino = ino;
    owner->sb.info.s_sb = &owner->sb;
    owner->sb.info.s_journal = NULL;
    if (ext4_has_feature_journal(&owner->sb) && ino == le32_to_cpu(volume->raw.s_journal_inum)) {
        owner->journal.j_inode = &owner->inode;
        owner->sb.info.s_journal = &owner->journal;
    }
    memcpy(owner->inode.info.i_data, refs, sizeof(owner->inode.info.i_data));
    result = ext4_check_blockref(__func__, __LINE__, &owner->inode,
                                owner->inode.info.i_data, EXT4_N_BLOCKS);
    if (result) { free(owner); return result; }
    *output = owner;
    return 0;
}

void ext_linux_indirect_close(struct ext_indirect *owner)
{
    unsigned i;
    if (!owner) return;
    for (i = 0; i < 3; ++i) free(owner->buffers[i]);
    free(owner);
}

int ext_linux_indirect_lookup(struct ext_indirect *owner, uint32_t logical, uint32_t count,
                              uint64_t *physical, uint32_t *length)
{
    struct ext4_map_blocks map = { .m_lblk = logical, .m_len = count };
    int result;
    *physical = 0;
    *length = 0;
    if (!owner || !count || count > INT_MAX ||
        (u64)logical + count > (1ULL << 32)) return -EINVAL;
    result = ext4_ind_map_blocks(NULL, &owner->inode, &map, 0);
    if (result < 0) return result;
    if (!map.m_len || map.m_len > count) return -EIO;
    if (map.m_flags & EXT4_MAP_MAPPED) {
        /* Archive source bounds also apply to the journal owner, whose
         * ext4_check_blockref deliberately bypasses reference validation. */
        if (!ext4_inode_block_valid(&owner->inode, map.m_pblk, map.m_len)) return -EFSCORRUPTED;
        *physical = map.m_pblk;
    }
    *length = map.m_len;
    return 0;
}

int ext_linux_indirect_map(struct ext_volume *volume, uint32_t ino, const void *refs,
                           uint32_t logical, uint32_t count,
                           const struct ext_indirect_services *services,
                           uint64_t *physical, uint32_t *length)
{
    struct ext_indirect *owner = NULL;
    int result = ext_linux_indirect_open(volume, ino, refs, services, &owner);
    *physical = 0;
    *length = 0;
    if (!result) result = ext_linux_indirect_lookup(owner, logical, count, physical, length);
    ext_linux_indirect_close(owner);
    return result;
}
