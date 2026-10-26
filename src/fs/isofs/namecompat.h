// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_ISO_NAME_COMPAT_H
#define SEVENZIP_ISO_NAME_COMPAT_H
#include "../../isonames.h"
#include "../fat/fat.h"
#include <stdio.h>
#ifdef _MSC_VER
#define __attribute__(value)
#endif
#include "../../include/uapi/linux/iso_fs.h"
#define PAGE_SIZE 4096
typedef uint16_t __be16;
#define be16_to_cpu __be16_to_cpu
static inline __be16 isofs_get_unaligned(const void *p) {
    __be16 value;
    memcpy(&value, p, sizeof(value));
    return value;
}
#define get_unaligned(p) isofs_get_unaligned(p)
static inline int isofs_utf16s_to_utf8s(const void *source, int length,
                                        enum utf16_endian endian, u8 *output, int capacity) {
    fat_wchar_t aligned[128];
    if (length < 0 || length > 127)
        return -EIO;
    memcpy(aligned, source, (size_t)length * sizeof(*aligned));
    return utf16s_to_utf8s(aligned, length, endian, output, capacity);
}
#define utf16s_to_utf8s isofs_utf16s_to_utf8s
struct isofs_name_super {
    struct nls_table *s_nls_iocharset;
};
struct inode {
    struct isofs_name_super *i_sb;
};
#define ISOFS_SB(sb) (sb)
int isofs_name_translate(struct iso_directory_record *, char *, struct inode *);
int get_acorn_filename(struct iso_directory_record *, char *, struct inode *);
int get_joliet_filename(struct iso_directory_record *, unsigned char *, struct inode *);
#endif
