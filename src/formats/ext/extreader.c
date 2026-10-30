/* SPDX-License-Identifier: GPL-2.0-only */
/* Archive-facing ownership and bounds for the transferred ext4 C readers. */
#include "formats/ext/ext4.h"
#include "formats/ext/xattr.h"
#include "formats/ext/ext4_extents.h"
#include "formats/ext/xattrread.h"
#include "crypto/fscrypt/fscryptreader.h"
#include "formats/ext/hashread.h"
#include "formats/ext/extreaderprivate.h"

_Static_assert(sizeof(struct ext4_super_block) == 1024, "ext4 superblock layout");
_Static_assert(sizeof(struct ext4_inode) == 160, "ext4 inode layout");

int ext_linux_open(const void *superblock, struct ext_volume **result)
{
    struct ext_volume *volume;
    struct ext4_sb_info *sbi;
    u32 bits, desc;
    *result = NULL;
    volume = calloc(1, sizeof(*volume));
    if (!volume) return -ENOMEM;
    memcpy(&volume->raw, superblock, sizeof(volume->raw));
    sbi = &volume->sb.info;
    sbi->s_sb = &volume->sb;
    sbi->s_es = &volume->raw;
    bits = le32_to_cpu(volume->raw.s_log_block_size);
    if (le16_to_cpu(volume->raw.s_magic) != 0xEF53 || bits > 6) goto corrupt;
    if (ext4_check_feature_compatibility(&volume->sb, &volume->raw)) goto corrupt;
    if (ext4_encoding_init(&volume->sb, &volume->raw)) goto corrupt;
    volume->sb.s_blocksize_bits = bits + 10;
    volume->sb.s_blocksize = 1024U << bits;
    if (ext4_hash_info_init(&volume->sb)) goto corrupt;
    if (le32_to_cpu(volume->raw.s_log_cluster_size) > 18) goto corrupt;
    sbi->s_blocks_per_group = le32_to_cpu(volume->raw.s_blocks_per_group);
    if (!sbi->s_blocks_per_group || ext4_handle_clustersize(&volume->sb)) goto corrupt;
    desc = ext4_has_feature_64bit(&volume->sb) ? le16_to_cpu(volume->raw.s_desc_size) : 32;
    if (desc < (ext4_has_feature_64bit(&volume->sb) ? 64U : 32U) ||
        desc > 1024 || (desc & (desc - 1))) goto corrupt;
    if (EXT4_INODES_PER_GROUP(&volume->sb) > volume->sb.s_blocksize * 8 ||
        EXT4_CLUSTERS_PER_GROUP(&volume->sb) > volume->sb.s_blocksize * 8) goto corrupt;
    sbi->s_desc_size = desc;
    sbi->s_desc_per_block = volume->sb.s_blocksize / desc;
    sbi->s_inodes_per_group = EXT4_INODES_PER_GROUP(&volume->sb);
    if (ext4_inode_info_init(&volume->sb, &volume->raw)) goto corrupt;
    volume->sb.s_maxbytes = ext4_max_size((int)volume->sb.s_blocksize_bits,
        ext4_has_feature_huge_file(&volume->sb));
    sbi->s_bitmap_maxbytes = ext4_max_bitmap_size((int)volume->sb.s_blocksize_bits,
        ext4_has_feature_huge_file(&volume->sb));
    if (EXT4_INODES_PER_GROUP(&volume->sb) % (volume->sb.s_blocksize / sbi->s_inode_size) ||
        (EXT4_CLUSTERS_PER_GROUP(&volume->sb) & 7)) goto corrupt;
    sbi->s_itb_per_group = EXT4_INODES_PER_GROUP(&volume->sb) / (volume->sb.s_blocksize / sbi->s_inode_size);
    if (ext4_has_feature_meta_bg(&volume->sb)) {
        u64 blocks = le32_to_cpu(volume->raw.s_blocks_count_lo);
        u32 per_group = le32_to_cpu(volume->raw.s_blocks_per_group);
        u32 first = le32_to_cpu(volume->raw.s_first_data_block);
        u64 groups;
        if (ext4_has_feature_64bit(&volume->sb)) blocks |= (u64)le32_to_cpu(volume->raw.s_blocks_count_hi) << 32;
        if (!per_group || blocks <= first) goto corrupt;
        groups = (blocks - first + per_group - 1) / per_group;
        if (le32_to_cpu(volume->raw.s_first_meta_bg) >
            (groups + sbi->s_desc_per_block - 1) / sbi->s_desc_per_block) goto corrupt;
    }
    if (ext4_has_feature_metadata_csum(&volume->sb) && volume->raw.s_checksum_type != 1) goto corrupt;
    if (!ext4_superblock_csum_verify(&volume->sb, &volume->raw)) goto corrupt;
    sbi->s_csum_seed = ext4_has_feature_csum_seed(&volume->sb) ?
        le32_to_cpu(volume->raw.s_checksum_seed) : ext4_chksum(~0U, volume->raw.s_uuid, 16);
    *result = volume;
    return 0;
corrupt:
    free(volume);
    return -EFSCORRUPTED;
}

void ext_linux_close(struct ext_volume *volume) {
    if (!volume) return;
    ext4_release_system_zone(&volume->sb);
    free(volume->sb.info.s_group_desc);
    free(volume->sb.info.s_archive_group_available);
    free(volume);
}

int ext_linux_geometry_check(struct ext_volume *volume, uint64_t input_size)
{
    volume->sb.s_archive_bytes = input_size;
    volume->sb.s_archive_partial = true;
    int error = ext4_check_geometry(&volume->sb, &volume->raw);
    if (!error && ext4_blocks_count(&volume->raw) > (input_size >> volume->sb.s_blocksize_bits))
        volume->sb.s_archive_limitations |= EXT_READ_SHORT_SOURCE;
    return error;
}

uint32_t ext_linux_read_limitations(const struct ext_volume *volume)
{
    return volume->sb.s_archive_limitations;
}

uint8_t ext_linux_group_availability(const struct ext_volume *volume, uint32_t group)
{
    const struct ext4_sb_info *sbi = &volume->sb.info;
    return group < sbi->s_groups_count && sbi->s_archive_group_available ?
        sbi->s_archive_group_available[group] : 0;
}

uint64_t ext_linux_descriptor_block(struct ext_volume *volume, unsigned int number)
{
    return descriptor_loc(&volume->sb, volume->sb.s_blocksize == 1024 ? 1 : 0, number);
}
int ext_linux_group_check(struct ext_volume *volume, uint32_t group, const void *descriptor)
{
    return ext4_group_desc_csum_verify(&volume->sb, group, (struct ext4_group_desc *)descriptor) ? 0 : -EFSBADCRC;
}
int ext_linux_bitmap_check(struct ext_volume *volume, uint32_t group, const void *descriptor,
                           const void *bitmap, int inode_bitmap)
{
    struct super_block *sb = &volume->sb;
    struct ext4_group_desc *desc = (struct ext4_group_desc *)descriptor;
    struct buffer_head bh = { .b_data = (char *)bitmap, .b_size = volume->sb.s_blocksize };
    ext4_fsblk_t block = inode_bitmap ? ext4_inode_bitmap(sb, desc) : ext4_block_bitmap(sb, desc);
    if (group >= ext4_get_groups_count(sb) ||
        block <= le32_to_cpu(volume->raw.s_first_data_block) || block >= ext4_blocks_count(&volume->raw))
        return -EFSCORRUPTED;
    int ok = inode_bitmap ? ext4_inode_bitmap_csum_verify(&volume->sb, (struct ext4_group_desc *)descriptor, &bh) :
        ext4_block_bitmap_csum_verify(&volume->sb, (struct ext4_group_desc *)descriptor, &bh);
    if (!ok) return -EFSBADCRC;
    if (!inode_bitmap && (ext4_valid_block_bitmap(sb, desc, group, &bh) ||
                           ext4_valid_block_bitmap_padding(sb, group, &bh)))
        return -EFSCORRUPTED;
    return 0;
}

static struct ext_time base_time(__le32 base)
{
    struct ext_time result = { (int32_t)le32_to_cpu(base), 0, 1 };
    return result;
}
static struct ext_time extra_time(__le32 base, __le32 extra)
{
    struct timespec64 ts = ext4_decode_extra_time(base, extra);
    struct ext_time result = { ts.tv_sec, (u32)ts.tv_nsec, 1 };
    return result;
}
#define DECODE_TIME(destination, field) do { \
    (destination) = base_time(raw->field); \
    if (EXT4_FITS_IN_INODE(raw, ei, field##_extra)) \
        (destination) = extra_time(raw->field, raw->field##_extra); \
} while (0)
int ext_linux_inode_open(struct ext_volume *volume, uint32_t ino, const void *data,
                        struct ext_node **result, struct ext_node_info *info)
{
    struct ext_node *node;
    struct inode *inode;
    struct ext4_inode_info *ei;
    struct ext4_inode *raw;
    struct ext4_iloc iloc;
    __le32 inum = cpu_to_le32(ino);
    unsigned int size = EXT4_INODE_SIZE(&volume->sb);
    int error;
    *result = NULL;
    memset(info, 0, sizeof(*info));
    node = calloc(1, sizeof(*node) + size);
    if (!node) return -ENOMEM;
    inode = &node->inode;
    inode->i_sb = &volume->sb;
    inode->i_ino = ino;
    inode->i_blkbits = volume->sb.s_blocksize_bits;
    inode->raw.b_data = (char *)(node + 1);
    inode->raw.b_size = size;
    memcpy(inode->raw.b_data, data, size);
    raw = (struct ext4_inode *)inode->raw.b_data;
    ei = &inode->info;
    ei->i_extra_isize = size > 128 ? le16_to_cpu(raw->i_extra_isize) : 0;
    if ((ei->i_extra_isize & 3) || 128U + ei->i_extra_isize > size) goto corrupt;
    ei->i_csum_seed = ext4_chksum(volume->sb.info.s_csum_seed, &inum, sizeof(inum));
    ei->i_csum_seed = ext4_chksum(ei->i_csum_seed, &raw->i_generation, sizeof(raw->i_generation));
    if (!ext4_inode_csum_verify(inode, raw, ei)) goto corrupt;
    inode->flags = le32_to_cpu(raw->i_flags);
    inode->i_mode = le16_to_cpu(raw->i_mode);
    ei->i_flags = inode->flags;
    inode->i_blocks = ext4_inode_blocks(raw, ei, inode);
    memcpy(ei->i_data, raw->i_block, sizeof(ei->i_data));
    inode->i_generation = le32_to_cpu(raw->i_generation);
    inode->i_nlink = le16_to_cpu(raw->i_links_count);
    inode->i_atime_sec = le32_to_cpu(raw->i_atime);
    inode->i_mtime_sec = le32_to_cpu(raw->i_mtime);
    inode->i_ctime_sec = le32_to_cpu(raw->i_ctime);
    inode->i_version = le32_to_cpu(raw->osd1.linux1.l_i_version);
    ei->i_file_acl = le32_to_cpu(raw->i_file_acl_lo);
    if (ext4_has_feature_64bit(&volume->sb))
        ei->i_file_acl |= (u64)le16_to_cpu(raw->osd2.linux2.l_i_file_acl_high) << 32;
    if ((inode->flags & (1U << 19)) && !(le32_to_cpu(volume->raw.s_feature_incompat) & 0x40)) goto corrupt;
    if ((inode->flags & (1U << 28)) && !(le32_to_cpu(volume->raw.s_feature_incompat) & 0x8000)) goto corrupt;
    if ((inode->flags & 0x1000U) && ext4_has_feature_metadata_csum(&volume->sb) &&
        !(le32_to_cpu(volume->raw.s_feature_compat) & 0x20)) goto corrupt;
    inode->i_size = ext4_isize(&volume->sb, raw);
    ext4_get_inode_loc(inode, &iloc);
    /* Selected original __ext4_iget extra-inode initialization. A zero disk
     * extra_isize does not hide the standard extended timestamp fields. */
    if (EXT4_INODE_SIZE(inode->i_sb) > EXT4_GOOD_OLD_INODE_SIZE) {
        if (ei->i_extra_isize == 0) {
            _Static_assert((sizeof(struct ext4_inode) & 3) == 0, "ext4 inode alignment");
            ei->i_extra_isize = sizeof(struct ext4_inode) - EXT4_GOOD_OLD_INODE_SIZE;
        } else {
            error = ext4_iget_extra_inode(inode, raw, ei);
            if (error) {
                /* Optional metadata must not hide independent file/directory
                 * data. Inline, encrypted and EA-inode decoding stays strict. */
                if (error == -ENOMEM || (inode->flags & ((1U << 28) | 0x800U | (1U << 21))))
                    goto fail;
                node->xattr_error = error;
                ei->i_state_flags = 0;
                ei->i_inline_off = 0;
                ei->i_inline_size = 0;
                inode->error = 0;
            }
        }
    }
    if (inode->flags & (1U << 28)) {
        if (!ei->i_inline_off || (inode->flags & (1U << 19))) goto corrupt;
        if (!S_ISDIR(inode->i_mode) && !S_ISLNK(inode->i_mode) && inode->i_size > ei->i_inline_size) goto corrupt;
        info->inline_size = ei->i_inline_size;
    }
    info->fast_symlink = ext4_inode_is_fast_symlink(inode);
    if (S_ISLNK(inode->i_mode)) {
        /* Selected original __ext4_iget symlink acceptance. The archive does
         * not run orphan recovery or publish a VFS cached link. */
        if (inode->flags & (0x20U | 0x10U)) goto corrupt;
        if (!(inode->flags & 0x800U) && info->fast_symlink) {
            if (inode->i_size == 0 || inode->i_size >= sizeof(ei->i_data) ||
                strnlen((char *)ei->i_data, (size_t)inode->i_size + 1) != inode->i_size)
                goto corrupt;
        }
    }
    if ((inode->flags & (1U << 19)) && !ext4_has_inline_data(inode) &&
        (S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
         (S_ISLNK(inode->i_mode) && !info->fast_symlink))) {
        error = ext4_ext_check_inode(inode);
        if (error) goto fail;
    }
    info->size = inode->i_size;
    info->checksum_seed = ei->i_csum_seed;
    info->has_xattrs = node->xattr_error || ext4_test_inode_state(inode, EXT4_STATE_XATTR) || ei->i_file_acl != 0;
    DECODE_TIME(info->atime, i_atime);
    DECODE_TIME(info->mtime, i_mtime);
    DECODE_TIME(info->change_time, i_ctime);
    if (EXT4_FITS_IN_INODE(raw, ei, i_crtime)) { DECODE_TIME(info->birth_time, i_crtime); }
    /* Inventory includes inactive reserved slots. Validate borrowed copies so
     * the archive's normal/special consumers can select the original contract. */
    {
        struct inode check = *inode;
        info->read_error = __ext4_iget(&check, EXT4_IGET_NORMAL, __func__, __LINE__);
        check = *inode;
        info->special_read_error = __ext4_iget(&check, EXT4_IGET_SPECIAL, __func__, __LINE__);
        check = *inode;
        info->ea_read_error = __ext4_iget(&check, EXT4_IGET_EA_INODE, __func__, __LINE__);
    }
    *result = node;
    return 0;
corrupt:
    error = -EFSCORRUPTED;
fail:
    free(node);
    return error;
}
#undef DECODE_TIME
void ext_linux_inode_close(struct ext_node *node) { free(node); }

struct inode *ext_xattr_iget(struct super_block *sb, unsigned long ino, ext4_iget_flags flags)
{
    const struct ext_xattr_services *services = EXT4_SB(sb)->xattr_services;
    struct ext_node *node = NULL;
    struct ext_node_info info;
    void *raw;
    int result;
    if (!services || !services->inode || ino > UINT32_MAX || !ext4_valid_inum(sb, ino))
        return ERR_PTR(-EFSCORRUPTED);
    raw = malloc(EXT4_INODE_SIZE(sb));
    if (!raw) return ERR_PTR(-ENOMEM);
    result = services->inode(services->context, (u32)ino, raw, EXT4_INODE_SIZE(sb));
    if (!result)
        result = ext_linux_inode_open((struct ext_volume *)sb, (u32)ino, raw, &node, &info);
    free(raw);
    if (!result)
        result = __ext4_iget(&node->inode, flags, __func__, __LINE__);
    if (result) { ext_linux_inode_close(node); return ERR_PTR(result); }
    return &node->inode;
}

void ext4_put_ea_inode(struct inode *inode) { ext_linux_inode_close((struct ext_node *)inode); }
int ext_linux_xattr_check(struct ext_node *node, uint64_t block, const void *data)
{
    struct buffer_head bh = { .b_data = (char *)data, .b_size = node->inode.i_sb->s_blocksize, .b_blocknr = block };
    return __ext4_xattr_check_block(&node->inode, &bh, __func__, __LINE__);
}

int ext_linux_xattr_visit(struct ext_node *node, const struct ext_xattr_services *services,
                          ext_xattr_emit emit, void *context)
{
    struct ext4_sb_info *sbi = EXT4_SB(node->inode.i_sb);
    const struct ext_xattr_services *previous = sbi->xattr_services;
    int result;
    sbi->xattr_services = services;
    if (node->xattr_error) {
        result = services && services->handle_error ?
            services->handle_error(services->context, node->xattr_error) : node->xattr_error;
        if (!result)
            result = ext4_xattr_visit_block(&node->inode, emit, context, true);
    } else
        result = ext4_xattr_visit(&node->inode, emit, context);
    sbi->xattr_services = previous;
    return result;
}
int ext_linux_fscrypt_context(struct ext_node *node, const struct ext_xattr_services *services,
                               void *output, size_t size)
{
    if (node->xattr_error) return node->xattr_error;
    struct ext4_sb_info *sbi = EXT4_SB(node->inode.i_sb);
    const struct ext_xattr_services *previous = sbi->xattr_services;
    sbi->xattr_services = services;
    int result = ext4_get_context(&node->inode, output, size);
    sbi->xattr_services = previous;
    return result;
}
void ext_linux_fscrypt_geometry(struct ext_volume *volume, struct sevenzip_fscrypt_geometry *geometry)
{
    memset(geometry, 0, sizeof(*geometry));
    geometry->block_bits = volume->sb.s_blocksize_bits;
    geometry->max_file_size = ext4_max_size((int)geometry->block_bits,
        (le32_to_cpu(volume->raw.s_feature_ro_compat) & 8) != 0);
    memcpy(geometry->uuid, volume->raw.s_uuid, 16);
    geometry->stable_inodes = (le32_to_cpu(volume->raw.s_feature_compat) & 0x800) != 0;
    geometry->has_32bit_inodes = 1;
    geometry->supports_subblock_units = 1;
}
int ext_linux_directory_hash(struct ext_volume *volume, uint32_t flags, uint64_t size,
                              const struct sevenzip_fscrypt_inode *crypt, int root_version, int linear,
                              const void *record, size_t record_size, uint32_t *hash, uint32_t *minor)
{
    const struct ext4_dir_entry_2 *de = record;
    struct inode dir = {.i_sb = &volume->sb, .flags = flags, .i_size = size, .fscrypt_owner = crypt};
    *hash = *minor = 0;
    if (record_size < 8 || de->name_len > record_size - 8) return -EFSCORRUPTED;
    if (!de->inode || !de->name_len ||
        (de->name[0] == '.' && (de->name_len == 1 || (de->name_len == 2 && de->name[1] == '.'))))
        return 0;
    /* Original ext4 readdir/htree rule: casefolded encrypted entries carry
     * hashes after the aligned ciphertext, independent of key availability. */
    if (ext4_hash_in_dirent(&dir)) {
        size_t offset = (8 + de->name_len + EXT4_DIR_ROUND) & ~EXT4_DIR_ROUND;
        if (offset + sizeof(struct ext4_dir_entry_hash) > record_size) return -EFSCORRUPTED;
        *hash = ext_load_le32((const u8 *)record + offset);
        *minor = ext_load_le32((const u8 *)record + offset + 4);
        return 0;
    }
    /* Original linear readdir supplies zero hashes unless stored in dirents. */
    if (linear) return 0;
    /* Original is_dx_dir() also selects a potentially indexed one-block dir. */
    if (!(le32_to_cpu(volume->raw.s_feature_compat) & 0x20) ||
        !((flags & 0x1000) || (size >> volume->sb.s_blocksize_bits) == 1 || (flags & (1U << 28))))
        return 0;
    int version = root_version < 0 ? volume->raw.s_def_hash_version : root_version;
    if (version < DX_HASH_LEGACY || version > DX_HASH_SIPHASH) return -EINVAL;
    u32 seed[4];
    for (unsigned i = 0; i < 4; ++i) seed[i] = le32_to_cpu(volume->raw.s_hash_seed[i]);
    u32 disk_flags = le32_to_cpu(volume->raw.s_flags);
    if (version <= DX_HASH_TEA && ((disk_flags & 2) || (!(disk_flags & 1) && CHAR_MIN == 0)))
        version += 3;
    struct dx_hash_info hinfo = {.hash_version = version, .seed = seed};
    int result = ext4fs_dirhash(&dir, de->name, de->name_len, &hinfo);
    if (!result) result = dir.error;
    if (!result) { *hash = hinfo.hash; *minor = hinfo.minor_hash; }
    return result;
}
int ext_linux_inline_read(struct ext_node *node, void *output, unsigned int size)
{
    struct ext4_iloc iloc;
    ext4_get_inode_loc(&node->inode, &iloc);
    return ext4_read_inline_data(&node->inode, output, size, &iloc);
}
int ext_linux_inline_directory(struct ext_node *node, ext_dir_emit emit, void *context)
{
    struct dir_private_info private = { .cookie = 1 };
    struct file file = { &node->inode, &private };
    struct dir_context ctx = { .emit = emit, .context = context };
    int has_inline = 1;
    int error = ext4_read_inline_dir(&file, &ctx, &has_inline);
    return error ? error : ctx.error ? ctx.error : node->inode.error;
}
int ext_linux_extent_check(struct ext_volume *volume, uint32_t ino, uint32_t seed,
                           uint32_t root_depth, const void *block, size_t size,
                           uint32_t depth, uint64_t physical, uint32_t logical)
{
    struct inode inode = { .i_sb = &volume->sb, .i_ino = ino, .info = { .i_csum_seed = seed } };
    struct ext4_extent_header *header = (struct ext4_extent_header *)block;
    if (size < sizeof(*header) || root_depth > 32 || depth > root_depth ||
        le16_to_cpu(header->eh_max) > (size - sizeof(*header)) / sizeof(struct ext4_extent))
        return -EFSCORRUPTED;
    ext_inode_hdr(&inode)->eh_depth = cpu_to_le16((u16)root_depth);
    return __ext4_ext_check(__func__, __LINE__, &inode, header, (int)depth, physical, logical);
}
int ext_linux_directory_check(struct ext_volume *volume, uint32_t seed, uint32_t flags,
                              uint32_t logical, const void *block)
{
    struct inode inode = { .i_sb = &volume->sb, .flags = flags, .info = { .i_csum_seed = seed } };
    struct buffer_head bh = { .b_data = (char *)block, .b_size = volume->sb.s_blocksize };
    unsigned int len = ext4_rec_len_from_disk(((struct ext4_dir_entry *)block)->rec_len, volume->sb.s_blocksize);
    if ((le32_to_cpu(volume->raw.s_feature_compat) & 0x20U) && (flags & 0x1000U)) {
        /* __ext4_read_dirblock identifies the root by logical block, not rec_len. */
        if (logical == 0)
            return ext4_dx_csum_verify(&inode, (struct ext4_dir_entry *)block) ? 0 : -EFSBADCRC;
        if (len == volume->sb.s_blocksize) {
            /* Linux notes that an empty leaf can resemble an index node. Linear
             * archive traversal has no INDEX caller hint: require one of the
             * original checksum validators rather than skipping verification. */
            return ext4_dirblock_csum_verify(&inode, &bh) ||
                ext4_dx_csum_verify(&inode, (struct ext4_dir_entry *)block) ? 0 : -EFSBADCRC;
        }
    }
    return ext4_dirblock_csum_verify(&inode, &bh) ? 0 : -EFSBADCRC;
}

uint8_t ext_linux_directory_type(struct ext_volume *volume, uint8_t filetype)
{
    return get_dtype(&volume->sb, filetype);
}

unsigned int ext_linux_directory_record_length(const void *record, unsigned int blocksize)
{
    const struct ext4_dir_entry_2 *entry = record;
    return ext4_rec_len_from_disk(entry->rec_len, blocksize);
}

int ext_linux_directory_entry_check(struct ext_volume *volume, uint32_t flags,
                                    const void *data, size_t size, unsigned int offset)
{
    struct inode inode = { .i_sb = &volume->sb, .flags = flags };
    struct buffer_head bh = { .b_data = (char *)data, .b_size = size };
    if (size > volume->sb.s_blocksize || offset > size || size - offset < 12)
        return -EFSCORRUPTED;
    return __ext4_check_dir_entry(__func__, __LINE__, &inode, NULL,
        (struct ext4_dir_entry_2 *)((char *)data + offset), &bh, (char *)data,
        (int)size, offset) ? -EFSCORRUPTED : 0;
}
