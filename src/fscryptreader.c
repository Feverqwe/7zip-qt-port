/* SPDX-License-Identifier: GPL-2.0-only */
/* Archive ownership boundary around the transferred Linux fscrypt readers. */
#include "fscryptreader.h"
#include "fs/crypto/fscrypt_private.h"

struct sevenzip_fscrypt_key { struct fscrypt_master_key key; };
struct sevenzip_fscrypt_inode {
    struct fscrypt_operations ops;
    struct super_block sb;
    struct inode inode;
    u8 context[sizeof(union fscrypt_context)];
    size_t context_size;
    bool context_present;
};
struct key_lookup {
    const struct sevenzip_fscrypt_key *const *keys;
    size_t count;
};

int sevenzip_fscrypt_key_create(unsigned type, const void *raw, size_t size,
                                const uint8_t *descriptor, struct sevenzip_fscrypt_key **out)
{
    if (!out) return -EINVAL;
    *out = NULL;
    if (size > FSCRYPT_MAX_RAW_KEY_SIZE) return -EINVAL;
    struct sevenzip_fscrypt_key *key = calloc(1, sizeof(*key));
    if (!key) return -ENOMEM;
    int err = fscrypt_prepare_master_key(&key->key, type, descriptor, raw, (unsigned)size);
    if (err) { sevenzip_fscrypt_key_free(key); return err; }
    *out = key;
    return 0;
}
void sevenzip_fscrypt_key_free(struct sevenzip_fscrypt_key *key)
{
    if (!key) return;
    memzero_explicit(key, sizeof(*key));
    free(key);
}

static struct fscrypt_master_key *find_key(void *opaque, const struct fscrypt_key_specifier *spec)
{
    const struct key_lookup *lookup = opaque;
    size_t len = spec->type == FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR ?
        FSCRYPT_KEY_DESCRIPTOR_SIZE : FSCRYPT_KEY_IDENTIFIER_SIZE;
    for (size_t i = 0; i < lookup->count; ++i) {
        const struct sevenzip_fscrypt_key *key = lookup->keys[i];
        if (key && key->key.mk_spec.type == spec->type && !memcmp(&key->key.mk_spec.u, &spec->u, len))
            return (struct fscrypt_master_key *)&key->key;
    }
    return NULL;
}
static bool has_stable_inodes(struct super_block *sb) { return sb->stable_inodes; }
static int get_context(struct inode *inode, void *out, size_t capacity)
{
    const struct sevenzip_fscrypt_inode *owner = inode->context_data;
    if (!owner->context_present) return -ENODATA;
    if (owner->context_size > capacity) return -ERANGE;
    memcpy(out, owner->context, owner->context_size);
    return (int)owner->context_size;
}

int sevenzip_fscrypt_inode_create(const struct sevenzip_fscrypt_inode_options *options,
                                  struct sevenzip_fscrypt_inode **out)
{
    if (!out) return -EINVAL;
    *out = NULL;
    if (!options || !options->geometry || (options->key_count && !options->keys) ||
        (options->context_size && !options->context)) return -EINVAL;
    const struct sevenzip_fscrypt_geometry *g = options->geometry;
    unsigned page_bits = g->page_bits ? g->page_bits : g->block_bits;
    if (g->block_bits < 9 || g->block_bits > 16 || page_bits < 9 || page_bits > 16 ||
        !g->max_file_size) return -EINVAL;
    struct sevenzip_fscrypt_inode *owner = calloc(1, sizeof(*owner));
    if (!owner) return -ENOMEM;
    owner->ops.has_stable_inodes = has_stable_inodes;
    owner->ops.has_32bit_inodes = g->has_32bit_inodes != 0;
    owner->ops.supports_subblock_data_units = g->supports_subblock_units != 0;
    owner->ops.get_context = get_context;
    owner->sb.s_cop = &owner->ops;
    owner->sb.s_maxbytes = g->max_file_size;
    owner->sb.s_blocksize = (u32)1 << g->block_bits;
    owner->sb.s_page_size = (u32)1 << page_bits;
    owner->sb.s_id = "archive";
    memcpy(owner->sb.s_uuid, g->uuid, sizeof(g->uuid));
    owner->sb.stable_inodes = g->stable_inodes != 0;
    owner->inode.i_sb = &owner->sb;
    owner->inode.i_ino = options->inode_number;
    owner->inode.i_mode = options->mode;
    owner->inode.i_blkbits = (u8)g->block_bits;
    owner->inode.casefolded = options->casefolded != 0;
    owner->inode.encrypted = true;
    owner->inode.context_data = owner;
    owner->context_present = options->context != NULL;
    owner->context_size = options->context_size;
    if (options->context_size)
        memcpy(owner->context, options->context, min(sizeof(owner->context), options->context_size));
    struct key_lookup lookup = {options->keys, options->key_count};
    owner->sb.find_key = find_key;
    owner->sb.key_context = &lookup;
    int err = fscrypt_get_encryption_info(&owner->inode, options->allow_unsupported != 0);
    /* Prepared CIs no longer borrow registry/context inputs after construction. */
    owner->sb.find_key = NULL;
    owner->sb.key_context = NULL;
    if (err) { sevenzip_fscrypt_inode_free(owner); return err; }
    *out = owner;
    return 0;
}
void sevenzip_fscrypt_inode_free(struct sevenzip_fscrypt_inode *owner)
{
    if (!owner) return;
    fscrypt_put_encryption_info(&owner->inode);
    memzero_explicit(owner, sizeof(*owner));
    free(owner);
}
int sevenzip_fscrypt_has_key(const struct sevenzip_fscrypt_inode *owner)
{ return owner && fscrypt_has_encryption_key(&owner->inode); }
int sevenzip_fscrypt_require_key(struct sevenzip_fscrypt_inode *owner)
{ return owner ? fscrypt_require_key(&owner->inode) : -EINVAL; }
int sevenzip_fscrypt_permitted_context(struct sevenzip_fscrypt_inode *parent,
                                        struct sevenzip_fscrypt_inode *child, unsigned mode)
{
    struct inode plain_parent = {0}, plain_child = {.i_mode = mode};
    return fscrypt_has_permitted_context(parent ? &parent->inode : &plain_parent,
                                         child ? &child->inode : &plain_child);
}
size_t sevenzip_fscrypt_data_unit_size(const struct sevenzip_fscrypt_inode *owner)
{
    if (!sevenzip_fscrypt_has_key(owner) || !S_ISREG(owner->inode.i_mode)) return 0;
    return (size_t)1 << owner->inode.i_crypt_info->ci_data_unit_bits;
}
int sevenzip_fscrypt_decrypt_unit(struct sevenzip_fscrypt_inode *owner, uint64_t index,
                                  const void *src, void *dst, size_t size)
{
    if (!owner || !src || !dst || !S_ISREG(owner->inode.i_mode)) return -EINVAL;
    int err = fscrypt_get_encryption_info(&owner->inode, false);
    if (err) return err;
    if (!sevenzip_fscrypt_has_key(owner)) return -ENOKEY;
    unsigned bits = owner->inode.i_crypt_info->ci_data_unit_bits;
    if (size != ((size_t)1 << bits) || index > ((owner->sb.s_maxbytes - 1) >> bits)) return -EINVAL;
    struct page input = {(u8 *)src}, output = {dst};
    return fscrypt_crypt_data_unit(owner->inode.i_crypt_info, FS_DECRYPT, index,
                                  &input, &output, (unsigned)size, 0);
}
int sevenzip_fscrypt_present_name(const struct sevenzip_fscrypt_inode *owner, uint32_t hash,
                                  uint32_t minor_hash, const void *cipher, size_t size,
                                  void *out, size_t capacity, size_t *out_size)
{
    if (!owner || !out_size || (!out && capacity) || (!cipher && size) || size > UINT16_MAX ||
        (!S_ISDIR(owner->inode.i_mode) && !S_ISLNK(owner->inode.i_mode)))
        return -EINVAL;
    *out_size = 0;
    struct fscrypt_str input = {(u8 *)cipher, (u32)size}, output = {0};
    int err = fscrypt_fname_alloc_buffer((u32)size, &output);
    if (err) return err;
    err = fscrypt_fname_disk_to_usr(&owner->inode, hash, minor_hash, &input, &output);
    if (!err) {
        *out_size = output.len;
        if (capacity <= output.len) err = -ERANGE;
        else { memcpy(out, output.name, output.len); ((u8 *)out)[output.len] = 0; }
    }
    fscrypt_fname_free_buffer(&output);
    return err;
}

int sevenzip_fscrypt_present_symlink(struct sevenzip_fscrypt_inode *owner,
                                     const void *payload, size_t size,
                                     void *out, size_t capacity, size_t *out_size)
{
    if (!owner || !out_size || (!out && capacity) || (!payload && size) ||
        size > UINT16_MAX + sizeof(struct fscrypt_symlink_data) || !S_ISLNK(owner->inode.i_mode))
        return -EINVAL;
    *out_size = 0;
    struct delayed_call done = {0};
    const char *target = fscrypt_get_symlink(&owner->inode, payload, (unsigned)size, &done);
    if (IS_ERR(target)) return (int)PTR_ERR(target);
    size_t len = strlen(target);
    *out_size = len;
    int err = 0;
    if (capacity <= len) err = -ERANGE;
    else memcpy(out, target, len + 1);
    do_delayed_call(&done);
    return err;
}

int sevenzip_fscrypt_hash_name(const struct sevenzip_fscrypt_inode *owner,
                               const void *name, size_t size, uint64_t *out)
{
    if (!owner || !out || (!name && size) || size > UINT32_MAX || !S_ISDIR(owner->inode.i_mode))
        return -EINVAL;
    if (!sevenzip_fscrypt_has_key(owner)) return -ENOKEY;
    if (!owner->inode.i_crypt_info->ci_dirhash_key_initialized) return -EINVAL;
    struct qstr input = {name, (u32)size};
    *out = fscrypt_fname_siphash(&owner->inode, &input);
    return 0;
}
