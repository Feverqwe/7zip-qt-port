// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 * Read-only userspace adaptation; see docs/upstream-port-audit/exfat-linux.md. */

#ifndef SEVENZIP_EXFAT_FS_H
#define SEVENZIP_EXFAT_FS_H
#include "exfat_compat.h"
#include "exfat_raw.h"
#define EXFAT_ROOT_INO 1

/*
 * exfat error flags
 */
enum exfat_error_mode {
    EXFAT_ERRORS_CONT,  /* ignore error and continue */
    EXFAT_ERRORS_PANIC, /* panic on error */
    EXFAT_ERRORS_RO,    /* remount r/o on error */
};

/*
 * exfat nls lossy flag
 */
enum {
    NLS_NAME_NO_LOSSY = 0,   /* no lossy */
    NLS_NAME_LOSSY = 1 << 0, /* just detected incorrect filename(s) */
};

#define EXFAT_HASH_BITS 8
#define EXFAT_HASH_SIZE (1UL << EXFAT_HASH_BITS)

/*
 * Type Definitions
 */
#define ES_2_ENTRIES 2
#define ES_ALL_ENTRIES 0

#define ES_IDX_FILE 0
#define ES_IDX_STREAM 1
#define ES_IDX_FIRST_FILENAME 2
#define EXFAT_FILENAME_ENTRY_NUM(name_len) DIV_ROUND_UP(name_len, EXFAT_FILE_NAME_LEN)
#define ES_IDX_LAST_FILENAME(name_len)                                                             \
    (ES_IDX_FIRST_FILENAME + EXFAT_FILENAME_ENTRY_NUM(name_len) - 1)

#define DIR_DELETED 0xFFFFFFF7

/* type values */
#define TYPE_UNUSED 0x0000
#define TYPE_DELETED 0x0001
#define TYPE_INVALID 0x0002
#define TYPE_CRITICAL_PRI 0x0100
#define TYPE_BITMAP 0x0101
#define TYPE_UPCASE 0x0102
#define TYPE_VOLUME 0x0103
#define TYPE_DIR 0x0104
#define TYPE_FILE 0x011F
#define TYPE_CRITICAL_SEC 0x0200
#define TYPE_STREAM 0x0201
#define TYPE_EXTEND 0x0202
#define TYPE_ACL 0x0203
#define TYPE_BENIGN_PRI 0x0400
#define TYPE_GUID 0x0401
#define TYPE_PADDING 0x0402
#define TYPE_ACLTAB 0x0403
#define TYPE_BENIGN_SEC 0x0800
#define TYPE_VENDOR_EXT 0x0801
#define TYPE_VENDOR_ALLOC 0x0802

#define MAX_CHARSET_SIZE 6  /* max size of multi-byte character */
#define MAX_NAME_LENGTH 255 /* max len of file name excluding NULL */
#define MAX_VFSNAME_BUF_SIZE ((MAX_NAME_LENGTH + 1) * MAX_CHARSET_SIZE)

#define EXFAT_HINT_NONE -1
#define EXFAT_MIN_SUBDIR 2

/*
 * helpers for fat entry.
 */
#define FAT_ENT_SIZE (4)
#define FAT_ENT_SIZE_BITS (2)
#define FAT_ENT_OFFSET_SECTOR(sb, loc)                                                             \
    (EXFAT_SB(sb)->FAT1_start_sector + (((u64)(loc) << FAT_ENT_SIZE_BITS) >> sb->s_blocksize_bits))
#define FAT_ENT_OFFSET_BYTE_IN_SECTOR(sb, loc)                                                     \
    (((loc) << FAT_ENT_SIZE_BITS) & (sb->s_blocksize - 1))

/*
 * helpers for bitmap.
 */
#define CLUSTER_TO_BITMAP_ENT(clu) ((clu) - EXFAT_RESERVED_CLUSTERS)
#define BITMAP_ENT_TO_CLUSTER(ent) ((ent) + EXFAT_RESERVED_CLUSTERS)
#define BITS_PER_SECTOR(sb) ((sb)->s_blocksize * BITS_PER_BYTE)
#define BITS_PER_SECTOR_MASK(sb) (BITS_PER_SECTOR(sb) - 1)
#define BITMAP_OFFSET_SECTOR_INDEX(sb, ent) ((ent / BITS_PER_BYTE) >> (sb)->s_blocksize_bits)
#define BITMAP_OFFSET_BIT_IN_SECTOR(sb, ent) (ent & BITS_PER_SECTOR_MASK(sb))
#define BITMAP_OFFSET_BYTE_IN_SECTOR(sb, ent) ((ent / BITS_PER_BYTE) & ((sb)->s_blocksize - 1))
#define IGNORED_BITS_REMAINED(clu, clu_base) ((1UL << ((clu) - (clu_base))) - 1)

#define ES_ENTRY_NUM(name_len) (ES_IDX_LAST_FILENAME(name_len) + 1)
/* 19 entries = 1 file entry + 1 stream entry + 17 filename entries */
#define ES_MAX_ENTRY_NUM ES_ENTRY_NUM(MAX_NAME_LENGTH)

/*
 * 19 entries x 32 bytes/entry = 608 bytes.
 * The 608 bytes are in 3 sectors at most (even 512 Byte sector).
 */
#define DIR_CACHE_SIZE (DIV_ROUND_UP(ES_MAX_ENTRY_NUM << DENTRY_SIZE_BITS, SECTOR_SIZE) + 1)

/* Superblock flags */
#define EXFAT_FLAGS_SHUTDOWN 1

struct exfat_dentry_namebuf {
    char *lfn;
    int lfnbuf_len; /* usually MAX_UNINAME_BUF_SIZE */
};

/* unicode name structure */
struct exfat_uni_name {
    /* +3 for null and for converting */
    unsigned short name[MAX_NAME_LENGTH + 3];
    u16 name_hash;
    unsigned char name_len;
};

/* directory structure */
struct exfat_chain {
    unsigned int dir;
    unsigned int size;
    unsigned char flags;
};

/* first empty entry hint information */
struct exfat_hint_femp {
    /* entry index of a directory */
    int eidx;
    /* count of continuous empty entry */
    int count;
    /* the cluster that first empty slot exists in */
    struct exfat_chain cur;
};

/* hint structure */
struct exfat_hint {
    unsigned int clu;
    union {
        unsigned int off; /* cluster offset */
        int eidx;         /* entry index */
    };
};

struct exfat_entry_set_cache {
    struct super_block *sb;
    unsigned int start_off;
    int num_bh;
    struct buffer_head *__bh[DIR_CACHE_SIZE];
    struct buffer_head **bh;
    unsigned int num_entries;
    bool modified;
};

#define IS_DYNAMIC_ES(es) ((es)->__bh != (es)->bh)

struct exfat_dir_entry {
    /* the cluster where file dentry is located */
    struct exfat_chain dir;
    /* the index of file dentry in ->dir */
    int entry;
    unsigned int type;
    unsigned int start_clu;
    unsigned char flags;
    unsigned short attr;
    loff_t size;
    loff_t valid_size;
    unsigned int num_subdirs;
    struct timespec64 atime;
    struct timespec64 mtime;
    struct timespec64 crtime;
    struct exfat_dentry_namebuf namebuf;
};

struct exfat_sb_info {
    unsigned long long num_sectors; /* num of sectors in volume */
    unsigned int num_clusters;      /* num of clusters in volume */
    unsigned int cluster_size;      /* cluster size in bytes */
    unsigned int cluster_size_bits;
    unsigned int sect_per_clus; /* cluster size in sectors */
    unsigned int sect_per_clus_bits;
    unsigned long long FAT1_start_sector; /* FAT1 start sector */
    unsigned long long FAT2_start_sector; /* FAT2 start sector */
    unsigned long long data_start_sector; /* data area start sector */
    unsigned long long data_start_bytes;
    unsigned int num_FAT_sectors;      /* num of FAT sectors */
    unsigned int root_dir;             /* root dir cluster */
    unsigned int dentries_per_clu;     /* num of dentries per cluster */
    unsigned int vol_flags;            /* volume flags */
    unsigned int vol_flags_persistent; /* volume flags to retain */
    struct buffer_head *boot_bh;       /* buffer_head of BOOT sector */

    unsigned int map_clu;          /* allocation bitmap start cluster */
    unsigned int map_sectors;      /* num of allocation bitmap sectors */
    struct buffer_head **vol_amap; /* allocation bitmap */

    unsigned short *vol_utbl; /* upcase table */

    unsigned int clu_srch_ptr;  /* cluster search pointer */
    unsigned int used_clusters; /* number of used clusters */
};

struct exfat_inode_info {
    unsigned int type, start_clu;
    unsigned char flags;
    struct exfat_hint hint_bmap;
};
struct inode {
    struct super_block *i_sb;
    struct exfat_inode_info info;
    loff_t i_size;
};
#define EXFAT_SB(sb) ((struct exfat_sb_info *)(sb)->s_fs_info)
#define EXFAT_I(inode) (&(inode)->info)
#define i_size_read(inode) ((inode)->i_size)
static inline bool exfat_is_last_sector_in_cluster(struct exfat_sb_info *sbi, sector_t sec) {
    return ((sec - sbi->data_start_sector + 1) & ((1 << sbi->sect_per_clus_bits) - 1)) == 0;
}

static inline sector_t exfat_cluster_to_sector(struct exfat_sb_info *sbi, unsigned int clus) {
    return ((sector_t)(clus - EXFAT_RESERVED_CLUSTERS) << sbi->sect_per_clus_bits) +
           sbi->data_start_sector;
}

static inline unsigned int exfat_sector_to_cluster(struct exfat_sb_info *sbi, sector_t sec) {
    return ((sec - sbi->data_start_sector) >> sbi->sect_per_clus_bits) + EXFAT_RESERVED_CLUSTERS;
}

static inline bool is_valid_cluster(struct exfat_sb_info *sbi, unsigned int clus) {
    return clus >= EXFAT_FIRST_CLUSTER && clus < sbi->num_clusters;
}

static inline loff_t exfat_ondisk_size(const struct inode *inode) {
    return inode->i_size; /* Read-only stream inode stores allocated bytes. */
}

static inline loff_t exfat_cluster_to_phys_bytes(struct exfat_sb_info *sbi, unsigned int clus) {
    return ((loff_t)(clus - EXFAT_RESERVED_CLUSTERS) << sbi->cluster_size_bits) +
           sbi->data_start_bytes;
}

/*
 * helpers for cluster size to byte conversion.
 */
static inline loff_t exfat_cluster_to_bytes(struct exfat_sb_info *sbi, u32 nr_clusters) {
    return (loff_t)nr_clusters << sbi->cluster_size_bits;
}

static inline blkcnt_t exfat_cluster_to_sectors(struct exfat_sb_info *sbi, u32 nr_clusters) {
    return (blkcnt_t)nr_clusters << (sbi->cluster_size_bits - 9);
}

static inline u32 exfat_bytes_to_cluster(struct exfat_sb_info *sbi, loff_t size) {
    return (u32)(size >> sbi->cluster_size_bits);
}

static inline u32 exfat_bytes_to_cluster_round_up(struct exfat_sb_info *sbi, loff_t size) {
    if (size <= 0)
        return 0;
    return (u32)((size - 1) >> sbi->cluster_size_bits) + 1;
}

static inline u32 exfat_cluster_offset(struct exfat_sb_info *sbi, loff_t off) {
    return off & (sbi->cluster_size - 1);
}

/*
 * helpers for block size to byte conversion.
 */
static inline loff_t exfat_block_to_bytes(struct super_block *sb, sector_t block) {
    return (loff_t)block << sb->s_blocksize_bits;
}

static inline sector_t exfat_bytes_to_block(struct super_block *sb, loff_t size) {
    return (sector_t)(size >> sb->s_blocksize_bits);
}

static inline sector_t exfat_bytes_to_block_round_up(struct super_block *sb, loff_t size) {
    if (size <= 0)
        return 0;
    return (sector_t)(((size - 1) >> sb->s_blocksize_bits) + 1);
}

static inline u32 exfat_block_offset(struct super_block *sb, loff_t off) {
    return (u32)(off & (sb->s_blocksize - 1));
}

/*
 * helpers for block size to dentry size conversion.
 */
static inline u32 exfat_bytes_to_dentries(loff_t b) { return (u32)(b >> DENTRY_SIZE_BITS); }

static inline u32 exfat_dentries_to_bytes(u32 dentry) { return dentry << DENTRY_SIZE_BITS; }

/*
 * helpers for cluster size to dentry size conversion.
 */
static inline u64 exfat_cluster_to_dentries(struct exfat_sb_info *sbi, u32 nr_clusters) {
    return (u64)nr_clusters << (sbi->cluster_size_bits - DENTRY_SIZE_BITS);
}

static inline u32 exfat_dentries_to_cluster(struct exfat_sb_info *sbi, u32 dentry) {
    return dentry >> (sbi->cluster_size_bits - DENTRY_SIZE_BITS);
}

int exfat_ent_get(struct super_block *, unsigned int, unsigned int *, struct buffer_head **);
static inline int exfat_cluster_walk(struct super_block *sb, unsigned int *clu, unsigned int step,
                                     int flags) {
    struct buffer_head *bh = NULL;

    if (flags == ALLOC_NO_FAT_CHAIN) {
        (*clu) += step;
        return 0;
    }

    while (step--) {
        if (exfat_ent_get(sb, *clu, clu, &bh))
            return -EIO;
    }
    brelse(bh);

    return 0;
}

static inline int exfat_chain_advance(struct super_block *sb, struct exfat_chain *chain,
                                      unsigned int step) {
    unsigned int clu = chain->dir;

    if (unlikely(chain->size < step))
        return -EIO;

    if (exfat_cluster_walk(sb, &clu, step, chain->flags))
        return -EIO;

    chain->size -= step;

    if (chain->size == 0 && chain->flags == ALLOC_NO_FAT_CHAIN)
        chain->dir = EXFAT_EOF_CLUSTER;
    else
        chain->dir = clu;

    return 0;
}

#define exfat_get_next_cluster(sb, pclu) exfat_cluster_walk(sb, pclu, 1, ALLOC_FAT_CHAIN)
int exfat_read_boot_sector(struct super_block *);
int exfat_verify_boot_region(struct super_block *);
int exfat_readdir(struct inode *, loff_t *, struct exfat_dir_entry *);
unsigned int exfat_get_entry_type(struct exfat_dentry *);
struct exfat_dentry *exfat_get_dentry(struct super_block *, struct exfat_chain *, int,
                                      struct buffer_head **);
struct exfat_dentry *exfat_get_dentry_cached(struct exfat_entry_set_cache *, int);
int exfat_get_dentry_set(struct exfat_entry_set_cache *, struct super_block *, struct exfat_chain *,
                         int, unsigned int);
int exfat_put_dentry_set(struct exfat_entry_set_cache *, int);
int exfat_count_num_clusters(struct super_block *, struct exfat_chain *, unsigned int *);
int exfat_load_bitmap(struct super_block *);
void exfat_free_bitmap(struct exfat_sb_info *);
bool exfat_test_bitmap(struct super_block *, unsigned int);
u16 exfat_calc_chksum16(void *, int, u16, int);
u32 exfat_calc_chksum32(void *, int, u32, int);
void exfat_chain_set(struct exfat_chain *, unsigned int, unsigned int, unsigned char);
void exfat_chain_dup(struct exfat_chain *, struct exfat_chain *);
int exfat_utf16_to_nls(struct super_block *, struct exfat_uni_name *, unsigned char *, int);
#endif
