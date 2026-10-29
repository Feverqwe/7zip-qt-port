/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SEVENZIP_FSCRYPT_READER_H
#define SEVENZIP_FSCRYPT_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_fscrypt_key;
struct sevenzip_fscrypt_inode;
enum sevenzip_fscrypt_key_type {
    SEVENZIP_FSCRYPT_V1_DESCRIPTOR = 1,
    SEVENZIP_FSCRYPT_V2_IDENTIFIER = 2
};
struct sevenzip_fscrypt_geometry {
    uint64_t max_file_size;
    uint8_t uuid[16];
    unsigned block_bits;
    unsigned page_bits; /* 0 selects a virtual page equal to the filesystem block. */
    int stable_inodes, has_32bit_inodes, supports_subblock_units;
};
struct sevenzip_fscrypt_inode_options {
    const struct sevenzip_fscrypt_geometry *geometry;
    uint64_t inode_number;
    unsigned mode; /* Linux on-disk inode mode, including file type. */
    int casefolded;
    const void *context; /* NULL means missing; non-NULL with size 0 means empty. */
    size_t context_size;
    const struct sevenzip_fscrypt_key *const *keys;
    size_t key_count;
    int allow_unsupported;
};
/* V1 needs an explicit 8-byte descriptor. V2 derives its identifier from raw. */
int sevenzip_fscrypt_key_create(unsigned type, const void *raw, size_t size,
                                const uint8_t *descriptor, struct sevenzip_fscrypt_key **out);
void sevenzip_fscrypt_key_free(struct sevenzip_fscrypt_key *key);
/* Construction snapshots metadata and prepares an independently owned key.
 * Input options, contexts and master keys need not outlive the result. */
int sevenzip_fscrypt_inode_create(const struct sevenzip_fscrypt_inode_options *,
                                  struct sevenzip_fscrypt_inode **out);
void sevenzip_fscrypt_inode_free(struct sevenzip_fscrypt_inode *);
int sevenzip_fscrypt_has_key(const struct sevenzip_fscrypt_inode *);
size_t sevenzip_fscrypt_data_unit_size(const struct sevenzip_fscrypt_inode *);
int sevenzip_fscrypt_decrypt_unit(struct sevenzip_fscrypt_inode *, uint64_t index, const void *src,
                                  void *dst, size_t size);
/* Presents directory names or symlink ciphertext; returns the length without
 * NUL in out_size. A successful buffer result is NUL-terminated. */
int sevenzip_fscrypt_present_name(const struct sevenzip_fscrypt_inode *, uint32_t hash,
                                  uint32_t minor_hash, const void *cipher, size_t size, void *out,
                                  size_t capacity, size_t *out_size);
/* Consumes the complete on-disk symlink payload, including its length header. */
int sevenzip_fscrypt_present_symlink(struct sevenzip_fscrypt_inode *, const void *payload,
                                     size_t size, void *out, size_t capacity, size_t *out_size);
/* Hash already-normalized filename bytes using the prepared casefold dir key. */
int sevenzip_fscrypt_hash_name(const struct sevenzip_fscrypt_inode *, const void *, size_t,
                               uint64_t *);
int sevenzip_fscrypt_require_key(struct sevenzip_fscrypt_inode *);
/* NULL represents an unencrypted inode; mode is used for an unencrypted child. */
int sevenzip_fscrypt_permitted_context(struct sevenzip_fscrypt_inode *parent,
                                       struct sevenzip_fscrypt_inode *child, unsigned child_mode);
#ifdef __cplusplus
}
#endif
#endif
