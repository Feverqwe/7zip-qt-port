// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_HFSPLUS_COMPAT_H
#define SEVENZIP_HFSPLUS_COMPAT_H
#include "../../hfsreader.h"
#include <errno.h>
#include <string.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef u16 __be16;
typedef u32 __be32;
typedef uint64_t __be64;
#define __packed
#define struct_group_attr(name, attr, ...)                                                         \
    union {                                                                                        \
        struct {                                                                                   \
            __VA_ARGS__                                                                            \
        };                                                                                         \
        struct {                                                                                   \
            __VA_ARGS__                                                                            \
        } name;                                                                                    \
    }
#pragma pack(push, 1)
#include "../../../third_party/linux/include/linux/hfs_common.h"
#pragma pack(pop)
#undef struct_group_attr
struct nls_table {
    int (*uni2char)(u16, unsigned char *, int);
};
struct super_block {
    struct nls_table *nls;
    unsigned long flags;
};
#define HFSPLUS_SB(sb) (sb)
#define HFSPLUS_SB_NODECOMPOSE 0
#define test_bit(bit, flags) ((*(flags) >> (bit)) & 1)
#define likely(x) (x)
#define min(a, b) ((a) < (b) ? (a) : (b))
#define pr_err(...) ((void)0)
#define EXPORT_SYMBOL_IF_KUNIT(symbol)
static inline u16 be16_to_cpu(u16 value) {
    const u8 *p = (const u8 *)&value;
    return (u16)((u16)p[0] << 8 | p[1]);
}
static inline u32 be32_to_cpu(u32 value) {
    const u8 *p = (const u8 *)&value;
    return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3];
}
extern u16 hfsplus_case_fold_table[], hfsplus_decompose_table[], hfsplus_compose_table[];
int hfsplus_strcasecmp(const struct hfsplus_unistr *, const struct hfsplus_unistr *);
int hfsplus_strcmp(const struct hfsplus_unistr *, const struct hfsplus_unistr *);
int hfsplus_uni2asc_str(struct super_block *, const struct hfsplus_unistr *, char *, int *);
int hfsplus_uni2asc_xattr_str(struct super_block *, const struct hfsplus_attr_unistr *, char *,
                              int *);
#endif
