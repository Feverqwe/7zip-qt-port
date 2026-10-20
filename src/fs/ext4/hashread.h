/* SPDX-License-Identifier: GPL-2.0-only */
/* Byte/rotation and opaque fscrypt services for the original ext4 hash bodies.
 * Archive readdir hashes stored ciphertext; Unicode lookup folding is omitted. */
#ifndef SEVENZIP_EXT4_HASH_READ_H
#define SEVENZIP_EXT4_HASH_READ_H
#include "../../fscryptreader.h"
#define IS_ENABLED(feature) 0
#define fallthrough ((void)0)
#define ext4_warning(...) ((void)0)
#define ext4_warning_inode(...) ((void)0)
#define DX_HASH_LEGACY 0
#define DX_HASH_HALF_MD4 1
#define DX_HASH_TEA 2
#define DX_HASH_LEGACY_UNSIGNED 3
#define DX_HASH_HALF_MD4_UNSIGNED 4
#define DX_HASH_TEA_UNSIGNED 5
#define DX_HASH_SIPHASH 6
#define EXT4_HTREE_EOF_32BIT ((1UL << 31) - 1)
struct dx_hash_info { u32 hash, minor_hash; int hash_version; u32 *seed; };
struct qstr { const void *name; size_t len; };
#define QSTR_INIT(name, length) {name, (size_t)(length)}
static inline u32 rol32(u32 value, unsigned shift)
{ return (value << shift) | (value >> (32 - shift)); }
static inline u32 get_unaligned_be32(const void *data)
{ const u8 *p = data; return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
static inline bool fscrypt_has_encryption_key(const struct inode *inode)
{ return sevenzip_fscrypt_has_key(inode->fscrypt_owner) != 0; }
static inline u64 fscrypt_fname_siphash(const struct inode *inode, const struct qstr *name)
{
    u64 hash = 0;
    ((struct inode *)inode)->error = sevenzip_fscrypt_hash_name(inode->fscrypt_owner,
                                                               name->name, name->len, &hash);
    return hash;
}
int ext4fs_dirhash(const struct inode *, const char *, int, struct dx_hash_info *);
#endif
