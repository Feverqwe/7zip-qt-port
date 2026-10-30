// SPDX-License-Identifier: GPL-2.0-only
// Read-only index/inode services for the original NTFS3 security read path.
#ifndef SEVENZIP_NTFS_SECURITY_COMPAT_H
#define SEVENZIP_NTFS_SECURITY_COMPAT_H
#include "compat/linux/include/asm-generic/bitops/__ffs.h"
#include "formats/ntfs/ntfs.h"
#include "formats/ntfs/ntfssecurity.h"
#define SECTOR_SHIFT 9
#define SECTOR_SIZE 512
#define E_NTFS_FIXUP 555
#define E_NTFS_CORRUPT 558
#define NTFS_DIRTY_DIRTY 1
#define GFP_NOFS 0
#define kmalloc(bytes, flags) malloc(bytes)
#define kzalloc_obj(type, flags) ((type *)calloc(1, sizeof(type)))
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#define IS_ALIGNED(v, a) (((v) & ((a) - 1)) == 0)
#define struct_size(p, member, count) (sizeof(*(p)) + (count) * sizeof((p)->member[0]))
#define init_rwsem(lock) ((void)(lock))
#define down_read(lock) ((void)(lock))
#define up_read(lock) ((void)(lock))
#define down_write(lock) ((void)(lock))
#define up_write(lock) ((void)(lock))
#define mutex_lock_nested(lock, kind) ((void)(lock))
#define ni_unlock(ni) ((void)(ni))
#define NTFS_INODE_MUTEX_SECURITY 0
#define WARN_ON_ONCE(value) ((void)(value))
typedef int (*NTFS_CMP_FUNC)(const void *, size_t, const void *, size_t, const void *);
enum index_mutex_classed {
    INDEX_MUTEX_I30,
    INDEX_MUTEX_SII,
    INDEX_MUTEX_SDH,
    INDEX_MUTEX_SO,
    INDEX_MUTEX_SQ,
    INDEX_MUTEX_SR,
    INDEX_MUTEX_TOTAL
};
struct rw_semaphore {
    int unused;
};
struct runs_tree {
    unsigned source;
};
struct ntfs_index {
    struct runs_tree bitmap_run, alloc_run;
    struct rw_semaphore run_lock;
    size_t version;
    u8 index_bits, idx2vbn_bits, vbn2vbo_bits, type;
};
struct ntfs_inode;
struct inode {
    struct ntfs_inode *owner;
    u64 size;
};
struct ntfs_sb_info {
    struct sevenzip_ntfs_security_input input;
    u32 cluster_size;
    unsigned cluster_bits;
    int error;
    bool fixup_corrected;
    struct {
        struct ntfs_inode *ni;
        struct ntfs_index index_sii;
    } security;
};
struct ATTRIB {
    struct {
        __le32 data_size;
    } res;
    const void *data;
};
struct mft_inode {
    int unused;
};
struct ntfs_inode {
    struct {
        struct ntfs_sb_info *sbi;
    } mi;
    struct {
        struct runs_tree run;
    } file;
    struct ntfs_index dir;
    struct ATTRIB root, bitmap;
    const struct sevenzip_ntfs_directory_input *directory_input;
    bool directory_unverified;
    int ni_lock;
    struct inode vfs_inode;
};
struct ntfs_buffers {
    int unused;
};
struct indx_node {
    struct ntfs_buffers nb;
    struct INDEX_BUFFER *index;
};
struct ntfs_fnd {
    int level;
    struct indx_node *nodes[20];
    struct NTFS_DE *de[20];
    struct NTFS_DE *root_de;
};
struct file_ra_state {
    int unused;
};
struct sevenzip_ntfs_security {
    struct ntfs_sb_info sbi;
    struct ntfs_inode ni;
    void *root;
};
extern const __le16 I30_NAME[4], SII_NAME[4], SDH_NAME[4], SO_NAME[2], SQ_NAME[2], SR_NAME[2];
static inline void nb_put(struct ntfs_buffers *nb) { (void)nb; }
static inline void run_close(struct runs_tree *run) { (void)run; }
static inline void ntfs_set_state(struct ntfs_sb_info *sbi, int state) {
    (void)state;
    sbi->error = -EINVAL;
}
static inline void _ntfs_bad_inode(void *inode) { (void)inode; }
static inline void *resident_data(const struct ATTRIB *attr) { return (void *)attr->data; }
static inline void *resident_data_ex(const struct ATTRIB *attr, size_t bytes) {
    return bytes <= le32_to_cpu(attr->res.data_size) ? resident_data(attr) : NULL;
}
static inline struct ATTRIB *ni_find_attr(struct ntfs_inode *ni, struct ATTRIB *prev,
                                          struct ATTR_LIST_ENTRY **le, enum ATTR_TYPE type,
                                          const __le16 *name, size_t length, void *vcn,
                                          struct mft_inode **mi) {
    (void)prev;
    (void)le;
    (void)vcn;
    (void)mi;
    if (length != ARRAY_SIZE(SII_NAME) ||
        (memcmp(name, SII_NAME, sizeof(SII_NAME)) && memcmp(name, I30_NAME, sizeof(I30_NAME))))
        return NULL;
    if (type == ATTR_ROOT)
        return &ni->root;
    return type == ATTR_BITMAP && ni->directory_input ? &ni->bitmap : NULL;
}
static inline int attr_load_runs_range(struct ntfs_inode *ni, enum ATTR_TYPE type,
                                       const __le16 *name, size_t length, struct runs_tree *run,
                                       u64 first, u64 end) {
    (void)ni;
    (void)type;
    (void)name;
    (void)length;
    (void)run;
    (void)first;
    (void)end;
    return 0; // The archive source already owns the complete validated mapping.
}
static inline int ntfs_read_run_nb(struct ntfs_sb_info *sbi, struct runs_tree *run, u64 offset,
                                   void *data, size_t size, void *nb) {
    (void)nb;
    const u64 limit = run->source ? sbi->input.allocation_size : sbi->input.sds_size;
    if (offset > limit || size > limit - offset || !sbi->input.read)
        return -EINVAL;
    return sbi->input.read(sbi->input.opaque, run->source, offset, data, size);
}
int ntfs_fix_post_read(struct NTFS_RECORD_HEADER *, size_t, bool);
static inline int ntfs_read_bh_ra(struct ntfs_sb_info *sbi, struct runs_tree *run, u64 offset,
                                  struct NTFS_RECORD_HEADER *header, size_t size,
                                  struct ntfs_buffers *nb, struct file_ra_state *ra) {
    (void)ra;
    int result = ntfs_read_run_nb(sbi, run, offset, header, size, nb);
    if (result)
        return result;
    result = ntfs_fix_post_read(header, size, false);
    if (!result && le16_to_cpu(header->fix_num) != size / SECTOR_SIZE + 1)
        sbi->fixup_corrected = true;
    return result;
}
static inline int ntfs_write_bh(struct ntfs_sb_info *sbi, struct NTFS_RECORD_HEADER *h,
                                struct ntfs_buffers *nb, int sync) {
    (void)h;
    (void)nb;
    (void)sync;
    sbi->fixup_corrected = true;
    return 0; // Fixups are corrected in the private read buffer; no disk writes.
}
static inline void put_indx_node(struct indx_node *node) {
    if (node) {
        kfree(node->index);
        nb_put(&node->nb);
        kfree(node);
    }
}
void fnd_clear(struct ntfs_fnd *);
static inline struct ntfs_fnd *fnd_get(void) { return kzalloc_obj(struct ntfs_fnd, GFP_NOFS); }
static inline void fnd_put(struct ntfs_fnd *fnd) {
    if (fnd) {
        fnd_clear(fnd);
        kfree(fnd);
    }
}
unsigned long _find_next_bit(const unsigned long *, unsigned long, unsigned long);
#define MINUS_ONE_T SIZE_MAX
static inline unsigned blksize_bits(unsigned size) {
    unsigned bits = SECTOR_SHIFT, blocks = size >> SECTOR_SHIFT;
    if (blocks > 1)
        for (--blocks; blocks; blocks >>= 1)
            ++bits;
    return bits;
}
int indx_init(struct ntfs_index *, struct ntfs_sb_info *, const struct ATTRIB *,
              enum index_mutex_classed);
struct INDEX_ROOT *indx_get_root(struct ntfs_index *, struct ntfs_inode *, struct ATTRIB **,
                                 struct mft_inode **);
int indx_read_ra(struct ntfs_index *, struct ntfs_inode *, CLST, struct indx_node **,
                 struct file_ra_state *);
static inline int indx_read(struct ntfs_index *indx, struct ntfs_inode *ni, CLST vbn,
                            struct indx_node **node) {
    return indx_read_ra(indx, ni, vbn, node, NULL);
}
int indx_find(struct ntfs_index *, struct ntfs_inode *, const struct INDEX_ROOT *, const void *,
              size_t, const void *, int *, struct NTFS_DE **, struct ntfs_fnd *);
int indx_used_bit(struct ntfs_index *, struct ntfs_inode *, size_t *);
int ntfs_directory_count(struct ntfs_inode *);
int ntfs_get_security_by_id(struct ntfs_sb_info *, __le32, struct SECURITY_DESCRIPTOR_RELATIVE **,
                            size_t *);
bool is_sd_valid(const struct SECURITY_DESCRIPTOR_RELATIVE *, u32);
#endif
