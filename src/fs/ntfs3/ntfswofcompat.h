// SPDX-License-Identifier: MIT
// Kernel cache/locking and run I/O boundary for the original WOF read functions.
#ifndef SEVENZIP_NTFS_WOF_COMPAT_H
#define SEVENZIP_NTFS_WOF_COMPAT_H
#include "ntfscompat.h"
#include "ntfs.h"
#include "lib/lib.h"
#include "../../ntfsdecoder.h"
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
typedef u64 pgoff_t;
struct folio { u64 index; u8 bytes[PAGE_SIZE]; };
struct runs_tree { int unused; };
struct ATTRIB {
    u8 non_res;
    __le16 flags;
    struct { __le32 data_size; } res;
    struct { __le64 data_size; } nres;
    const void *data;
};
struct super_block;
struct ntfs_mount_options;
struct ntfs_sb_info {
    const u16 *upcase;
    sevenzip_ntfs_read_fn read;
    void *opaque;
    struct super_block *sb;
    struct ntfs_mount_options *options;
    struct { u64 max_size; } reparse;
    struct {
        void *lzx, *xpress;
        int mtx_lzx, mtx_xpress;
    } compress;
};
int ntfs_cmp_names(const __le16 *, size_t, const __le16 *, size_t, const u16 *, bool);
struct ntfs_inode;
struct inode {
    struct super_block *i_sb;
    struct ntfs_inode *ntfs;
    u64 i_size;
};
struct ntfs_inode {
    struct ntfs_inode *base;
    unsigned ni_flags;
    struct ATTRIB *reparse;
    struct { struct ntfs_sb_info *sbi; } mi;
    struct inode vfs_inode;
    struct { int run_lock; struct folio *offs_folio; struct runs_tree run; } file;
    struct { struct ATTR_LIST_ENTRY *le; size_t size; } attr_list;
};
extern const __le16 WOF_NAME[17];
#define down_write(p) ((void)(p))
#define up_write(p) ((void)(p))
#define mutex_lock(p) ((void)(p))
#define mutex_unlock(p) ((void)(p))
#define folio_lock(p) ((void)(p))
#define folio_unlock(p) ((void)(p))
static inline struct folio *folio_alloc(int flags, int order) {
    (void)flags; (void)order; return calloc(1, sizeof(struct folio));
}
static inline void *folio_address(struct folio *p) { return p->bytes; }
static inline void _ntfs_bad_inode(void *inode) { (void)inode; }
static inline void *resident_data(const struct ATTRIB *a) { return (void *)a->data; }
static inline int attr_load_runs_range(struct ntfs_inode *ni, int type, const __le16 *name,
                                      size_t name_len, struct runs_tree *run, u64 from, u64 to) {
    (void)ni; (void)type; (void)name; (void)name_len; (void)run; (void)from; (void)to;
    return 0; /* ADS mappings are validated and owned by the incoming stream. */
}
static inline int ntfs_read_run(struct ntfs_sb_info *sbi, struct runs_tree *run,
                               void *data, u64 from, u64 size) {
    (void)run;
    return sbi->read(sbi->opaque, from, data, (size_t)size);
}
int attr_wof_frame_info(struct ntfs_inode *, struct ATTRIB *, struct runs_tree *,
                      u64, u64, u8, u32 *, u64 *);
static inline int ntfs_wof_bounds(u64 start, u64 end, u64 frame, u64 frames,
                                  u8 bits, u64 size, u64 table) {
    if (table > size || start > size - table)
        return -EINVAL;
    if (frame < frames && (end < start || end > size - table || end - start > ((u64)1 << bits)))
        return -EINVAL;
    return 0;
}
#endif
