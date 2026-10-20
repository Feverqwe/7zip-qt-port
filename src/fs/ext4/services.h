/* SPDX-License-Identifier: GPL-2.0-only */
/* Immutable-image services for the transferred Linux ext4 readers. */
#ifndef SEVENZIP_EXT4_SERVICES_H
#define SEVENZIP_EXT4_SERVICES_H
#define EXT4_GOOD_OLD_INODE_SIZE 128
#define EXT4_OS_LINUX 0
#define EXT4_ROOT_INO 2
#define EXT4_BOOT_LOADER_INO 5
#define EXT4_NDIR_BLOCKS 12
#define EXT4_ORPHAN_FS 0x0004
#define EXT4_INODE_EXTENTS 19
#define EXT4_FIRST_INO(sb) (EXT4_SB(sb)->s_first_ino)
#define EXT4_GOOD_OLD_REV 0
#define EXT4_GOOD_OLD_FIRST_INO 11
#define EXT4_EXTRA_TIMESTAMP_MAX (((int64_t)1 << 34) - 1 + INT32_MIN)
#define EXT4_NON_EXTRA_TIMESTAMP_MAX INT32_MAX
#define EXT4_TIMESTAMP_MIN INT32_MIN
#define NSEC_PER_SEC 1000000000L
#define is_power_of_2(n) ((n) && !((n) & ((n) - 1)))
#define i_size_read(inode) ((inode)->i_size)
#define IS_CASEFOLDED(inode) (((inode)->flags & 0x40000000U) != 0)
#define S_ISCHR(mode) (((mode) & 0170000) == 0020000)
#define S_ISBLK(mode) (((mode) & 0170000) == 0060000)
#define S_ISFIFO(mode) (((mode) & 0170000) == 0010000)
#define S_ISSOCK(mode) (((mode) & 0170000) == 0140000)
#define DIV_ROUND_UP_ULL(value, divisor) (((value) + (divisor) - 1) / (divisor))
#define __ext4_error(...) ((void)0)
loff_t ext4_max_bitmap_size(int, int);
#define EXT4_MAX_REC_LEN 65535
#define EXT4_DIR_ROUND 3
#define EXT4_FT_DIR_CSUM 0xDE
#define EXT4_MIN_INLINE_DATA_SIZE 60
#define EXT4_INLINE_DOTDOT_SIZE 4
#define EXT4_XATTR_SYSTEM_DATA "data"
#define DT_DIR 4
#define i_checksum_lo osd2.linux2.l_i_checksum_lo
#define i_blocks_high osd2.linux2.l_i_blocks_high
typedef u32 ext4_lblk_t;
#define S_ISLNK(mode) (((mode) & 0170000) == 0120000)
#define S_ISREG(mode) (((mode) & 0170000) == 0100000)
#define S_ISDIR(mode) (((mode) & 0170000) == 0040000)
struct ext4_sb_info {
    struct super_block *s_sb;
    struct ext4_super_block *s_es;
    u32 s_csum_seed, s_desc_size, s_desc_per_block, s_inode_size;
    u32 s_groups_count, s_gdb_count, s_itb_per_group;
    u32 s_first_ino, s_want_extra_isize;
    loff_t s_bitmap_maxbytes;
    u32 s_mount_state;
    void *s_group_desc;
    struct ext4_system_blocks *s_system_blks;
    const struct ext_block_services *block_services;
    struct journal_s *s_journal;
    const struct ext_xattr_services *xattr_services;
};
struct fsverity_operations;
struct ext4_extent_header;
struct sevenzip_asymmetric_trust;
struct super_block { unsigned int s_blocksize, s_blocksize_bits; struct ext4_sb_info info; const struct fsverity_operations *s_vop; loff_t s_maxbytes; int64_t s_time_min, s_time_max; unsigned int s_time_gran; };
struct ext4_inode_info { u32 i_csum_seed, i_inline_size, i_flags; u16 i_extra_isize, i_inline_off; unsigned long i_state_flags; u64 i_file_acl; __le32 i_data[EXT4_N_BLOCKS]; };
struct inode {
    struct super_block *i_sb;
    u32 i_ino, flags;
    u32 i_flags;
    u64 i_size;
    u64 i_blocks;
    u16 i_mode;
    u32 i_generation, i_nlink, i_atime_sec, i_mtime_sec, i_ctime_sec, i_version;
    int error;
    bool bad_inode;
    struct ext4_inode_info info;
    struct buffer_head raw;
    unsigned int i_blkbits;
    struct inode *i_mapping;
    void *verity_context;
    const struct sevenzip_asymmetric_trust *verity_trust;
    bool verity_require_signatures;
    const struct sevenzip_fscrypt_inode *fscrypt_owner;
};
typedef struct journal_s { struct inode *j_inode; } journal_t;
struct ext4_iloc { struct buffer_head *bh; unsigned int offset; };
struct dir_private_info { u64 cookie; };
struct file { struct inode *inode; void *private_data; };
struct dir_context { loff_t pos; ext_dir_emit emit; void *context; int error; };
#define EXT4_SB(sb) (&(sb)->info)
#define EXT4_I(inode) (&(inode)->info)
#define EXT4_INODE_SIZE(sb) (EXT4_SB(sb)->s_inode_size)
#define EXT4_BLOCK_SIZE(sb) ((sb)->s_blocksize)
#define EXT4_INODES_PER_GROUP(sb) le32_to_cpu(EXT4_SB(sb)->s_es->s_inodes_per_group)
#define EXT4_CLUSTERS_PER_GROUP(sb) le32_to_cpu(EXT4_SB(sb)->s_es->s_clusters_per_group)
#define EXT4_FITS_IN_INODE(raw, ei, field) \
    (offsetof(struct ext4_inode, field) + sizeof((raw)->field) <= EXT4_GOOD_OLD_INODE_SIZE + (ei)->i_extra_isize)
#define EXT_FEATURE(name, field, bit) \
    static inline bool ext4_has_feature_##name(const struct super_block *sb) { \
        return (le32_to_cpu(EXT4_SB(sb)->s_es->s_feature_##field) & (1U << bit)) != 0; }
EXT_FEATURE(metadata_csum, ro_compat, 10)
EXT_FEATURE(gdt_csum, ro_compat, 4)
EXT_FEATURE(sparse_super, ro_compat, 0)
EXT_FEATURE(sparse_super2, compat, 9)
EXT_FEATURE(64bit, incompat, 7)
EXT_FEATURE(meta_bg, incompat, 4)
EXT_FEATURE(ea_inode, incompat, 10)
EXT_FEATURE(csum_seed, incompat, 13)
EXT_FEATURE(filetype, incompat, 1)
EXT_FEATURE(encrypt, incompat, 16)
EXT_FEATURE(journal, compat, 2)
EXT_FEATURE(flex_bg, incompat, 9)
EXT_FEATURE(huge_file, ro_compat, 3)
EXT_FEATURE(largedir, incompat, 14)
EXT_FEATURE(casefold, incompat, 17)
EXT_FEATURE(extra_isize, ro_compat, 6)
int ext4_inode_info_init(struct super_block *, struct ext4_super_block *);
#define EXT4_INODE_HUGE_FILE 18
#define ext4_test_inode_flag(inode, flag) (((inode)->flags & (1U << (flag))) != 0)
#define EXT4_CLUSTER_SIZE(sb) (1024U << le32_to_cpu(EXT4_SB(sb)->s_es->s_log_cluster_size))
int ext4_inode_is_fast_symlink(struct inode *);
u64 ext4_inode_blocks(struct ext4_inode *, struct ext4_inode_info *, struct inode *);
int ext4_ext_check_inode(struct inode *);
int __ext4_ext_check(const char *, unsigned int, struct inode *, struct ext4_extent_header *, int, ext4_fsblk_t, ext4_lblk_t);
#define ext4_error_inode_err(inode, ...) ((inode)->error = -EFSCORRUPTED)
static inline loff_t ext4_isize(struct super_block *sb,
				struct ext4_inode *raw_inode)
{
	if (ext4_has_feature_largedir(sb) ||
	    S_ISREG(le16_to_cpu(raw_inode->i_mode)))
		return ((loff_t)le32_to_cpu(raw_inode->i_size_high) << 32) |
			le32_to_cpu(raw_inode->i_size_lo);

	return (loff_t) le32_to_cpu(raw_inode->i_size_lo);
}
static inline loff_t ext4_get_maxbytes(struct inode *inode)
{
	if (ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS))
		return inode->i_sb->s_maxbytes;
	return EXT4_SB(inode->i_sb)->s_bitmap_maxbytes;
}
static inline bool is_special_ino(struct super_block *sb, unsigned long ino)
{
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;

	return (ino < EXT4_FIRST_INO(sb) && ino != EXT4_ROOT_INO) ||
		ino == le32_to_cpu(es->s_usr_quota_inum) ||
		ino == le32_to_cpu(es->s_grp_quota_inum) ||
		ino == le32_to_cpu(es->s_prj_quota_inum) ||
		ino == le32_to_cpu(es->s_orphan_file_inum);
}
#define EXT4_BLOCKS_PER_GROUP(sb) le32_to_cpu(EXT4_SB(sb)->s_es->s_blocks_per_group)
#define EXT4_BG_INODE_ZEROED 4
#define ext4_debug(...) ((void)0)
#define ext4_msg(...) ((void)0)
#define sb_rdonly(sb) true
#define ext4_lock_group(sb, group) ((void)0)
#define ext4_unlock_group(sb, group) ((void)0)
int ext4_check_descriptors(struct super_block *, ext4_fsblk_t, ext4_group_t *);
static inline u64 ext4_blocks_count(struct ext4_super_block *es) {
    return le32_to_cpu(es->s_blocks_count_lo) |
        ((le32_to_cpu(es->s_feature_incompat) & EXT4_FEATURE_INCOMPAT_64BIT) ?
        (u64)le32_to_cpu(es->s_blocks_count_hi) << 32 : 0);
}
#define EXT4_DESC_SIZE(sb) (EXT4_SB(sb)->s_desc_size)
#define EXT4_DESC_PER_BLOCK(sb) (EXT4_SB(sb)->s_desc_per_block)
#define EXT4_MIN_DESC_SIZE_64BIT 64
static inline ext4_group_t ext4_get_groups_count(struct super_block *sb) {
    return EXT4_SB(sb)->s_groups_count;
}
static inline struct ext4_group_desc *ext4_get_group_desc(struct super_block *sb, u32 group, void *bh) {
    (void)bh;
    return (struct ext4_group_desc *)((u8 *)EXT4_SB(sb)->s_group_desc + (size_t)group * EXT4_DESC_SIZE(sb));
}
ext4_fsblk_t ext4_block_bitmap(struct super_block *, struct ext4_group_desc *);
ext4_fsblk_t ext4_inode_bitmap(struct super_block *, struct ext4_group_desc *);
ext4_fsblk_t ext4_inode_table(struct super_block *, struct ext4_group_desc *);
unsigned long ext4_bg_num_gdb(struct super_block *, ext4_group_t);
unsigned int ext4_num_base_meta_blocks(struct super_block *, ext4_group_t);
int ext4_setup_system_zone(struct super_block *);
void ext4_release_system_zone(struct super_block *);
int ext4_sb_block_valid(struct super_block *, struct inode *, ext4_fsblk_t, unsigned int);
int ext4_inode_block_valid(struct inode *, ext4_fsblk_t, unsigned int);
int ext4_check_blockref(const char *, unsigned int, struct inode *, __le32 *, unsigned int);
static inline bool ext4_has_unknown_ext4_incompat_features(struct super_block *sb) {
    return (EXT4_SB(sb)->s_es->s_feature_incompat & cpu_to_le32(~EXT4_FEATURE_INCOMPAT_SUPP)) != 0;
}
static inline bool ext4_has_group_desc_csum(struct super_block *sb) {
    return ext4_has_feature_metadata_csum(sb) || ext4_has_feature_gdt_csum(sb);
}
static inline ext4_fsblk_t ext4_group_first_block_no(struct super_block *sb, ext4_group_t group) {
    return (ext4_fsblk_t)group * le32_to_cpu(EXT4_SB(sb)->s_es->s_blocks_per_group) +
        le32_to_cpu(EXT4_SB(sb)->s_es->s_first_data_block);
}
static inline int ext4_valid_inum(struct super_block *sb, unsigned long ino)
{
	return ino == EXT4_ROOT_INO ||
		(ino >= EXT4_FIRST_INO(sb) &&
		 ino <= le32_to_cpu(EXT4_SB(sb)->s_es->s_inodes_count));
}
static inline bool ext4_hash_in_dirent(const struct inode *inode) {
    return (inode->flags & 0x40000800U) == 0x40000800U;
}
static inline unsigned int ext4_dir_rec_len(__u8 name_len, const struct inode *dir) {
    int rec_len = name_len + 8 + EXT4_DIR_ROUND;
    if (dir && ext4_hash_in_dirent(dir)) rec_len += sizeof(struct ext4_dir_entry_hash);
    return rec_len & ~EXT4_DIR_ROUND;
}
static inline unsigned int ext4_rec_len_from_disk(__le16 dlen, unsigned int blocksize) {
    unsigned int len = le16_to_cpu(dlen);
    if (len == EXT4_MAX_REC_LEN || len == 0) return blocksize;
    return (len & 65532) | ((len & 3) << 16);
}
#define EXT4_DIRENT_TAIL(block, size) ((struct ext4_dir_entry_tail *)((char *)(block) + (size) - sizeof(struct ext4_dir_entry_tail)))
#define ext4_raw_inode(iloc) ((struct ext4_inode *)((iloc)->bh->b_data + (iloc)->offset))
static inline int ext4_get_inode_loc(struct inode *inode, struct ext4_iloc *iloc) {
    iloc->bh = &inode->raw; iloc->offset = 0; return 0;
}
/* State is initialized while the archive owns the unpublished inode, then
 * consumed by the original read paths; no concurrent kernel bitops apply. */
static inline int ext4_test_inode_state(struct inode *inode, int bit) {
    return (EXT4_I(inode)->i_state_flags >> bit) & 1UL;
}
static inline void ext4_set_inode_state(struct inode *inode, int bit) {
    EXT4_I(inode)->i_state_flags |= 1UL << bit;
}
#define ext4_has_inline_data(inode) (((inode)->flags & (1U << 28)) && (inode)->info.i_inline_off)
#define file_inode(file) ((file)->inode)
#define inode_eq_iversion(inode, cookie) ((cookie) == 1)
#define inode_query_iversion(inode) 1
#define EXT4_ERROR_INODE(inode, ...) ((inode)->error = -EFSCORRUPTED)
#define __ext4_error_inode(inode, ...) ((inode)->error = -EFSCORRUPTED)
#define ext4_error_inode(inode, ...) ((inode)->error = -EFSCORRUPTED)
#define __ext4_warning_inode(inode, ...) ((inode)->error = -EFSCORRUPTED)
#define ext4_error_file(file, ...) ((file)->inode->error = -EFSCORRUPTED)
static inline bool dir_emit(struct dir_context *ctx, const char *name, int len, u64 ino, unsigned int type) {
    ctx->error = ctx->emit(ctx->context, (u32)ino, type, name, len);
    return ctx->error == 0;
}
static inline unsigned int get_dtype(struct super_block *sb, unsigned int type) {
    static const unsigned char types[] = {0, 8, 4, 2, 6, 1, 12, 10};
    return ext4_has_feature_filetype(sb) && type < sizeof(types) ? types[type] : 0;
}
int ext4_bg_has_super(struct super_block *, ext4_group_t);
int ext4_superblock_csum_verify(struct super_block *, struct ext4_super_block *);
int ext4_check_feature_compatibility(struct super_block *, struct ext4_super_block *);
int ext4_group_desc_csum_verify(struct super_block *, u32, struct ext4_group_desc *);
ext4_fsblk_t descriptor_loc(struct super_block *, ext4_fsblk_t, int);
int ext4_inode_bitmap_csum_verify(struct super_block *, struct ext4_group_desc *, struct buffer_head *);
int ext4_block_bitmap_csum_verify(struct super_block *, struct ext4_group_desc *, struct buffer_head *);
int ext4_inode_csum_verify(struct inode *, struct ext4_inode *, struct ext4_inode_info *);
int ext4_iget_extra_inode(struct inode *, struct ext4_inode *, struct ext4_inode_info *);
int ext4_find_inline_data_nolock(struct inode *);
int ext4_read_inline_data(struct inode *, void *, unsigned int, struct ext4_iloc *);
int ext4_read_inline_dir(struct file *, struct dir_context *, int *);
int __ext4_check_dir_entry(const char *, unsigned int, struct inode *, struct file *, struct ext4_dir_entry_2 *, struct buffer_head *, char *, int, unsigned int);
#define ext4_check_dir_entry(dir, file, de, bh, buf, size, offset) \
    __ext4_check_dir_entry(__func__, __LINE__, dir, file, de, bh, buf, size, offset)
struct ext4_extent_header;
int ext4_extent_block_csum_verify(struct inode *, struct ext4_extent_header *);
int ext4_dirblock_csum_verify(struct inode *, struct buffer_head *);
int ext4_dx_csum_verify(struct inode *, struct ext4_dir_entry *);
typedef enum {
    EXT4_IGET_NORMAL = 0, EXT4_IGET_SPECIAL = 0x0001, EXT4_IGET_HANDLE = 0x0002,
    EXT4_IGET_BAD = 0x0004, EXT4_IGET_EA_INODE = 0x0008, EXT4_IGET_NOWAIT = 0x0010
} ext4_iget_flags;
int check_igot_inode(struct inode *, ext4_iget_flags, const char *, unsigned int);
#define EXT4_EA_INODE_FL 0x00200000U
#define is_bad_inode(inode) ((inode)->bad_inode)
#define make_bad_inode(inode) ((inode)->bad_inode = true)
int __ext4_iget(struct inode *, ext4_iget_flags, const char *, unsigned int);
#endif
