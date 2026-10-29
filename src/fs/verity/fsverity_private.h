/* SPDX-License-Identifier: GPL-2.0 */
/*
 * fs-verity: read-only file-based authenticity protection
 *
 * Copyright 2019 Google LLC
 */

#ifndef SEVENZIP_FSVERITY_PRIVATE_H
#define SEVENZIP_FSVERITY_PRIVATE_H
#include "userspace.h"
#define FS_VERITY_MAX_LEVELS 8
struct fsverity_hash_alg {
    const char *name;         /* crypto API name, e.g. sha256 */
    unsigned int digest_size; /* digest size in bytes, e.g. 32 for SHA-256 */
    unsigned int block_size;  /* block size in bytes, e.g. 64 for SHA-256 */
    /*
     * The HASH_ALGO_* constant for this algorithm.  This is different from
     * FS_VERITY_HASH_ALG_*, which uses a different numbering scheme.
     */
    enum hash_algo algo_id;
};

union fsverity_hash_ctx {
    struct sha256_ctx sha256;
    struct sha512_ctx sha512;
};

struct merkle_tree_params {
    const struct fsverity_hash_alg *hash_alg; /* the hash algorithm */
    /* initial hash state if salted, NULL if unsalted */
    const union fsverity_hash_ctx *hashstate;
    unsigned int digest_size;      /* same as hash_alg->digest_size */
    unsigned int block_size;       /* size of data and tree blocks */
    unsigned int hashes_per_block; /* number of hashes per tree block */
    unsigned int blocks_per_page;  /* PAGE_SIZE / block_size */
    u8 log_digestsize;             /* log2(digest_size) */
    u8 log_blocksize;              /* log2(block_size) */
    u8 log_arity;                  /* log2(hashes_per_block) */
    u8 log_blocks_per_page;        /* log2(blocks_per_page) */
    unsigned int num_levels;       /* number of levels in Merkle tree */
    u64 tree_size;                 /* Merkle tree size in bytes */
    fsverity_index_t tree_pages;   /* Merkle tree size in pages */

    /* the hash of an all-zeroes block */
    u8 zero_digest[FS_VERITY_MAX_DIGEST_SIZE];

    /*
     * Starting block index for each tree level, ordered from leaf level (0)
     * to root level ('num_levels - 1')
     */
    fsverity_index_t level_start[FS_VERITY_MAX_LEVELS];
};

struct fsverity_info {
    struct merkle_tree_params tree_params;
    union {
        u8 root_hash[FS_VERITY_MAX_DIGEST_SIZE];

        /*
         * Used for final freeing of the struct, in which case root_hash
         * is no longer needed.  Specifically, the only fields needed
         * besides the rcu_head are those needed by fsverity_free_info()
         * itself, and those that can be accessed by fsverity_get_info()
         * (rhash_head and inode) during the RCU-protected rhashtable
         * chain search trying to find another inode's fsverity_info.
         */
    };
    u8 file_digest[FS_VERITY_MAX_DIGEST_SIZE];
    struct inode *inode;
    unsigned long *hash_block_verified;
};

extern const struct fsverity_hash_alg fsverity_hash_algs[];
const struct fsverity_hash_alg *fsverity_get_hash_alg(const struct inode *, unsigned int);
union fsverity_hash_ctx *fsverity_prepare_hash_state(const struct fsverity_hash_alg *, const u8 *,
                                                     size_t);
void fsverity_hash_block(const struct merkle_tree_params *, const void *, u8 *);
void fsverity_hash_buffer(const struct fsverity_hash_alg *, const void *, size_t, u8 *);
int fsverity_get_descriptor(struct inode *, struct fsverity_descriptor **);
struct fsverity_info *fsverity_create_info(struct inode *, struct fsverity_descriptor *);
void fsverity_free_info(struct fsverity_info *);
bool fsverity_verify_blocks(struct fsverity_info *, struct folio *, size_t, size_t);
int fsverity_init_merkle_tree_params(struct merkle_tree_params *, const struct inode *,
                                     unsigned int, unsigned int, const u8 *, size_t);
int fsverity_verify_signature(const struct fsverity_info *, const u8 *, size_t);
extern const struct fsverity_operations ext4_read_verityops;
#endif
