/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Selected on-disk directory type from include/uapi/linux/iso_fs.h. */
#ifndef SEVENZIP_ISO_DISK_RECORD_H
#define SEVENZIP_ISO_DISK_RECORD_H
#define ISODCL(from, to) (to - from + 1)
struct iso_directory_record {
    __u8 length[ISODCL(1, 1)];          /* 711 */
    __u8 ext_attr_length[ISODCL(2, 2)]; /* 711 */
    __u8 extent[ISODCL(3, 10)];         /* 733 */
    __u8 size[ISODCL(11, 18)];          /* 733 */
    __u8 date[ISODCL(19, 25)];          /* 7 by 711 */
    __u8 flags[ISODCL(26, 26)];
    __u8 file_unit_size[ISODCL(27, 27)];         /* 711 */
    __u8 interleave[ISODCL(28, 28)];             /* 711 */
    __u8 volume_sequence_number[ISODCL(29, 32)]; /* 723 */
    __u8 name_len[ISODCL(33, 33)];               /* 711 */
    char name[];
} __attribute__((packed));
#endif
