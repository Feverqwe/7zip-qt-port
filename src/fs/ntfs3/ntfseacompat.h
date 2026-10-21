// SPDX-License-Identifier: MIT
// Per-inode attribute and read services for the original NTFS3 EA read functions.
#ifndef SEVENZIP_NTFS_EA_COMPAT_H
#define SEVENZIP_NTFS_EA_COMPAT_H
#include "ntfs.h"
#include "../../ntfsea.h"
#ifndef ENODATA
#define ENODATA 61
#endif
#define static_assert(expr) _Static_assert(expr, #expr)
#define struct_size(p, member, count) (sizeof(*(p)) + (count) * sizeof((p)->member[0]))
#define kmalloc(size, flags) malloc(size)
#define uid_t u32
#define gid_t u32
#define NI_FLAG_EA 0x20
#define NTFS_DIRTY_DIRTY 1
#define NTFS_DIRTY_ERROR 2
#define ni_lock(ni) ((void)(ni))
#define ni_unlock(ni) ((void)(ni))
struct ATTRIB {
    const void *data;
    u64 bytes;
    u8 non_res;
    __le16 flags;
};
struct ntfs_sb_info {
    const struct sevenzip_ntfs_ea_input *input;
    u32 ea_max_size;
    int error;
};
struct ntfs_inode;
struct inode {
    struct ntfs_inode *owner;
    u32 i_uid, i_gid, i_rdev;
    u16 i_mode;
    bool permissions;
};
struct ntfs_inode {
    struct { struct ntfs_sb_info *sbi; } mi;
    struct inode vfs_inode;
    struct ATTRIB info, ea;
    u32 ni_flags;
};
struct runs_tree { int unused; };
#define run_init(run) ((void)(run))
#define run_close(run) ((void)(run))
static inline struct ntfs_inode *ntfs_i(struct inode *inode) { return inode->owner; }
static inline void i_uid_write(struct inode *inode, u32 uid) {
    inode->i_uid = uid;
    inode->permissions = true;
}
static inline void i_gid_write(struct inode *inode, u32 gid) { inode->i_gid = gid; }
static inline void ntfs_set_state(struct ntfs_sb_info *sbi, int state) {
    (void)state;
    sbi->error = -EINVAL;
}
static inline void *resident_data_ex(const struct ATTRIB *attr, size_t bytes) {
    return !attr->non_res && bytes <= attr->bytes ? (void *)attr->data : NULL;
}
static inline u64 attr_size(const struct ATTRIB *attr) { return attr->bytes; }
static inline struct ATTRIB *ni_find_attr(struct ntfs_inode *ni, struct ATTRIB *previous,
                                          struct ATTR_LIST_ENTRY **le, enum ATTR_TYPE type,
                                          const void *name, size_t length, void *vcn, void *mi) {
    (void)previous; (void)le; (void)name; (void)length; (void)vcn; (void)mi;
    return type == ATTR_EA_INFO ? &ni->info : &ni->ea;
}
static inline int attr_load_runs_range(struct ntfs_inode *ni, enum ATTR_TYPE type,
                                       const void *name, size_t length, struct runs_tree *run,
                                       u64 start, u64 end) {
    (void)ni; (void)type; (void)name; (void)length; (void)run; (void)start; (void)end;
    return 0; // The incoming SDK stream owns its validated complete attribute mapping.
}
static inline int ntfs_read_run_nb(struct ntfs_sb_info *sbi, struct runs_tree *run,
                                   u64 offset, void *data, size_t size, void *nb) {
    (void)run; (void)nb;
    const struct sevenzip_ntfs_ea_input *input = sbi->input;
    if (offset > input->data_size || size > input->data_size - offset || !input->read)
        return sbi->error = -EINVAL;
    const int result = input->read(input->opaque, offset, data, size);
    if (result)
        sbi->error = result;
    return result;
}
#endif
