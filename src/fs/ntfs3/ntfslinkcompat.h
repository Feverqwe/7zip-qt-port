// SPDX-License-Identifier: GPL-2.0-only
// Read-only inode, dentry and Unicode services for the original NTFS3 link path.
#ifndef SEVENZIP_NTFS_LINK_COMPAT_H
#define SEVENZIP_NTFS_LINK_COMPAT_H
#include "ntfswofcompat.h"
#include "../nls/types.h"
#include "../../ntfslink.h"
#define PATH_MAX 4096
#define static_assert(expr) _Static_assert(expr, #expr)
#define noinline
#define kmalloc(bytes, flags) malloc(bytes)
#define IS_ERR(pointer) ((pointer) == NULL)
#define ntfs_err(...) ((void)0)
#define ntfs_warn(...) ((void)0)
#define NI_FLAG_COMPRESSED_MASK 0x0000000f
#define NI_FLAG_DEDUPLICATED 0x00000010
enum REPARSE_SIGN { REPARSE_NONE, REPARSE_COMPRESSED, REPARSE_DEDUPLICATED, REPARSE_LINK };
struct ntfs_mount_options { struct nls_table *nls; };
struct super_block { struct ntfs_sb_info *s_fs_info; };
struct dentry { const char *path; };
static inline struct ntfs_inode *ntfs_i(struct inode *inode) { return inode->ntfs; }
static inline char *dentry_path_raw(const struct dentry *de, char *buffer, size_t capacity) {
    size_t bytes = strlen(de->path);
    if (bytes >= capacity) return NULL;
    memcpy(buffer, de->path, bytes + 1);
    return buffer;
}
static inline void *resident_data_ex(const struct ATTRIB *attr, size_t bytes) {
    return bytes <= le32_to_cpu(attr->res.data_size) ? resident_data(attr) : NULL;
}
static inline struct ATTRIB *ni_find_attr(struct ntfs_inode *ni, void *prev, void *list,
        enum ATTR_TYPE type, const __le16 *name, size_t length, void *vcn, void *mi) {
    (void)prev; (void)list; (void)name; (void)vcn; (void)mi;
    return type == ATTR_REPARSE && !length ? ni->reparse : NULL;
}
static inline void run_init(struct runs_tree *run) { memset(run, 0, sizeof(*run)); }
static inline void run_close(struct runs_tree *run) { (void)run; }
static inline int attr_load_runs_vcn(struct ntfs_inode *ni, enum ATTR_TYPE type,
        const __le16 *name, size_t length, struct runs_tree *run, u64 vcn) {
    (void)ni; (void)type; (void)name; (void)length; (void)run; (void)vcn;
    return 0;
}
static inline int ntfs_read_run_nb(struct ntfs_sb_info *sbi, struct runs_tree *run,
        u64 offset, void *data, size_t size, void *nb) {
    (void)run; (void)nb;
    return sbi->read ? sbi->read(sbi->opaque, offset, data, size) : -EINVAL;
}
static inline void ni_set_ext_compress_bits(struct ntfs_inode *ni, u8 bits) {
    ni->ni_flags |= (bits - 0xb) & NI_FLAG_COMPRESSED_MASK;
}
static inline char *hex_byte_pack(char *out, unsigned value) {
    static const char digits[] = "0123456789abcdef";
    *out++ = digits[(value >> 4) & 15]; *out++ = digits[value & 15]; return out;
}
/* Kernel paths can use unaligned byte offsets; keep the NLS input aligned. */
static inline int ntfs_link_utf16s_to_utf8s(const fat_wchar_t *input, int length,
        enum utf16_endian endian, u8 *output, int capacity) {
    fat_wchar_t aligned[8192];
    if (length < 0 || length > (int)ARRAY_SIZE(aligned)) return 0;
    memcpy(aligned, input, (size_t)length * sizeof(*aligned));
    return utf16s_to_utf8s(aligned, length, endian, output, capacity);
}
#define utf16s_to_utf8s ntfs_link_utf16s_to_utf8s
enum REPARSE_SIGN ni_parse_reparse(struct ntfs_inode *, struct ATTRIB *, struct REPARSE_DATA_BUFFER *);
int ntfs_utf16_to_nls(struct ntfs_sb_info *, const __le16 *, u32, u8 *, int);
int ntfs_archive_readlink(struct dentry *, struct inode *, char *, int);
#endif
