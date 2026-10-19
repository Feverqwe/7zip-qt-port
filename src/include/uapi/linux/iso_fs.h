/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Selected on-disk directory and High Sierra descriptor types from include/uapi/linux/iso_fs.h. */
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
#define HS_STANDARD_ID "CDROM"

struct hs_volume_descriptor {
    __u8 foo[ISODCL(1, 8)];  /* 733 */
    __u8 type[ISODCL(9, 9)]; /* 711 */
    char id[ISODCL(10, 14)];
    __u8 version[ISODCL(15, 15)]; /* 711 */
    __u8 data[ISODCL(16, 2048)];
};

struct hs_primary_descriptor {
    __u8 foo[ISODCL(1, 8)];  /* 733 */
    __u8 type[ISODCL(9, 9)]; /* 711 */
    __u8 id[ISODCL(10, 14)];
    __u8 version[ISODCL(15, 15)];                  /* 711 */
    __u8 unused1[ISODCL(16, 16)];                  /* 711 */
    char system_id[ISODCL(17, 48)];                /* achars */
    char volume_id[ISODCL(49, 80)];                /* dchars */
    __u8 unused2[ISODCL(81, 88)];                  /* 733 */
    __u8 volume_space_size[ISODCL(89, 96)];        /* 733 */
    __u8 unused3[ISODCL(97, 128)];                 /* 733 */
    __u8 volume_set_size[ISODCL(129, 132)];        /* 723 */
    __u8 volume_sequence_number[ISODCL(133, 136)]; /* 723 */
    __u8 logical_block_size[ISODCL(137, 140)];     /* 723 */
    __u8 path_table_size[ISODCL(141, 148)];        /* 733 */
    __u8 type_l_path_table[ISODCL(149, 152)];      /* 731 */
    __u8 unused4[ISODCL(153, 180)];                /* 733 */
    __u8 root_directory_record[ISODCL(181, 214)];  /* 9.1 */
};

#endif
