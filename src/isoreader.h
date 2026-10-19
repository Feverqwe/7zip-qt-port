// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_ISO_READER_H
#define SEVENZIP_ISO_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct timespec64 {
    int64_t tv_sec;
    long tv_nsec;
};
struct super_block;
struct sevenzip_iso_inode {
    struct super_block *i_sb;
    unsigned i_blkbits;
    uint32_t i_mode, i_nlink, i_uid, i_gid, i_rdev;
    uint64_t i_size, i_blocks;
    struct timespec64 atime, mtime, ctime;
    unsigned long i_iget5_block, i_iget5_offset;
    uint32_t i_first_extent;
    unsigned char i_file_format, i_format_parm[3];
};
struct sevenzip_iso_state {
    int rock, rock_offset;
    uint32_t block_size, volume_blocks;
    void *context;
    int (*read)(void *, uint64_t, void *, size_t);
    int (*relocate)(void *, uint32_t, struct sevenzip_iso_inode *);
};
struct sevenzip_iso_record {
    const unsigned char *system_use;
    size_t system_use_size;
    unsigned char date[7];
    unsigned char flags;
    uint64_t size, block;
    unsigned long record_block, record_offset;
};
int sevenzip_iso_rock(struct sevenzip_iso_state *, const struct sevenzip_iso_record *,
                      int relocated, struct sevenzip_iso_inode *, char name[256], char link[4096]);
int sevenzip_iso_relocated(struct sevenzip_iso_state *, uint32_t, struct sevenzip_iso_inode *);
int isofs_fill_super(struct sevenzip_iso_state *, int, int, int, uint32_t);
#ifdef __cplusplus
}
#endif
#endif
