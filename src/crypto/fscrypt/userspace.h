/* SPDX-License-Identifier: GPL-2.0-only */
/* Immutable inode/volume services for the Linux fscrypt read-side code. */
#ifndef SEVENZIP_FSCRYPT_USERSPACE_H
#define SEVENZIP_FSCRYPT_USERSPACE_H
#include "compat/linux/include/uapi/linux/fscrypt.h"
#include "crypto/modes/skciphercompat.h"
#include <linux/count_zeros.h>
#include <linux/siphash.h>
#define SECTOR_SHIFT 9
#define FSCRYPT_CONTENTS_ALIGNMENT 16
#ifndef NAME_MAX
#define NAME_MAX 255
#endif
#ifndef EUCLEAN
#define EUCLEAN 117
#endif
#define GFP_NOFS 0
#define kmalloc(size, flags) malloc(size)
#define kfree free
#define max_t(t, a, b) ((t)(a) > (t)(b) ? (t)(a) : (t)(b))
#define U32_MAX UINT32_MAX
#define fscrypt_warn(inode, ...) ((void)(inode))
#define fscrypt_err(inode, ...) ((void)(inode))
#define BUG() abort()
#ifndef ENOPKG
#define ENOPKG 65
#endif
#ifndef ENODATA
#define ENODATA 61
#endif
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & 0170000) == 0100000)
#define S_ISDIR(mode) (((mode) & 0170000) == 0040000)
#define S_ISLNK(mode) (((mode) & 0170000) == 0120000)
#define S_IFMT 0170000
#endif
#define WARN_ONCE(e, ...) crypto_warn_on(e)
/* All archive providers are synchronous CPU implementations. */
#define crypto_sync_skcipher crypto_skcipher
#define crypto_alloc_sync_skcipher crypto_alloc_skcipher
#define crypto_free_sync_skcipher crypto_free_skcipher
#define crypto_sync_skcipher_set_flags crypto_skcipher_set_flags
#define crypto_sync_skcipher_setkey crypto_skcipher_setkey
static inline unsigned crypto_sync_skcipher_ivsize(const struct crypto_skcipher *tfm) {
    return tfm->ivsize;
}
#define FSCRYPT_CRYPTOAPI_MASK 0
struct fscrypt_inode_info;
struct fscrypt_master_key;
struct super_block;
struct inode;
struct fscrypt_operations {
    bool (*has_stable_inodes)(struct super_block *);
    bool has_32bit_inodes;
    bool supports_subblock_data_units;
    int (*get_context)(struct inode *, void *, size_t);
};
struct super_block {
    const struct fscrypt_operations *s_cop;
    u64 s_maxbytes;
    u32 s_blocksize, s_page_size;
    const char *s_id;
    u8 s_uuid[16];
    bool stable_inodes;
    struct fscrypt_master_key *(*find_key)(void *, const struct fscrypt_key_specifier *);
    void *key_context;
};
struct inode {
    struct super_block *i_sb;
    u64 i_ino;
    unsigned i_mode;
    u8 i_blkbits;
    bool casefolded;
    bool encrypted;
    struct fscrypt_inode_info *i_crypt_info;
    void *context_data;
};
#define IS_ENCRYPTED(inode) ((inode)->encrypted)
struct delayed_call {
    void (*fn)(void *);
    void *arg;
};
static inline void kfree_link(void *p) { free(p); }
static inline void set_delayed_call(struct delayed_call *done, void (*fn)(void *), void *arg) {
    done->fn = fn;
    done->arg = arg;
}
static inline void do_delayed_call(struct delayed_call *done) {
    if (done->fn)
        done->fn(done->arg);
}
struct fscrypt_str {
    u8 *name;
    u32 len;
};
struct qstr {
    const u8 *name;
    u32 len;
};
#define FSTR_TO_QSTR(str) ((struct qstr){(str)->name, (str)->len})
/* Original include/linux/fs.h predicate, with a byte-buffer type boundary. */
static inline bool name_is_dot_dotdot(const void *bytes, size_t len) {
    const char *name = bytes;
    return len && unlikely(name[0] == '.') && (len == 1 || (len == 2 && name[1] == '.'));
}
static inline struct fscrypt_master_key *
fscrypt_find_master_key(struct super_block *sb, const struct fscrypt_key_specifier *spec) {
    return sb->find_key ? sb->find_key(sb->key_context, spec) : NULL;
}
#define IS_CASEFOLDED(inode) ((inode)->casefolded)
void *memchr_inv(const void *, int, size_t);
#endif
