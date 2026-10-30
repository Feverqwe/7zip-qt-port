// SPDX-License-Identifier: GPL-2.0
/*
  File: fs/ext4/xattr.h

  On-disk format of extended attributes for the ext4 filesystem.

  (C) 2001 Andreas Gruenbacher, <a.gruenbacher@computer.org>
*/

#ifndef SEVENZIP_EXT4_XATTR_H
#define SEVENZIP_EXT4_XATTR_H
/* Magic value in attribute blocks */
#define EXT4_XATTR_MAGIC 0xEA020000

/* Maximum number of references to one attribute block */
#define EXT4_XATTR_REFCOUNT_MAX 1024

/* Name indexes */
#define EXT4_XATTR_INDEX_USER 1
#define EXT4_XATTR_INDEX_POSIX_ACL_ACCESS 2
#define EXT4_XATTR_INDEX_POSIX_ACL_DEFAULT 3
#define EXT4_XATTR_INDEX_TRUSTED 4
#define EXT4_XATTR_INDEX_LUSTRE 5
#define EXT4_XATTR_INDEX_SECURITY 6
#define EXT4_XATTR_INDEX_SYSTEM 7
#define EXT4_XATTR_INDEX_RICHACL 8
#define EXT4_XATTR_INDEX_ENCRYPTION 9
#define EXT4_XATTR_NAME_ENCRYPTION_CONTEXT "c"
#define EXT4_XATTR_INDEX_HURD 10 /* Reserved for Hurd */

struct ext4_xattr_header {
    __le32 h_magic;      /* magic number for identification */
    __le32 h_refcount;   /* reference count */
    __le32 h_blocks;     /* number of disk blocks used */
    __le32 h_hash;       /* hash value of all attributes */
    __le32 h_checksum;   /* crc32c(uuid+blknum+xattrblock) */
    __u32 h_reserved[3]; /* zero right now */
};

struct ext4_xattr_ibody_header {
    __le32 h_magic; /* magic number for identification */
};

struct ext4_xattr_entry {
    __u8 e_name_len;     /* length of name */
    __u8 e_name_index;   /* attribute name index */
    __le16 e_value_offs; /* offset in disk block of value */
    __le32 e_value_inum; /* inode in which the value is stored */
    __le32 e_value_size; /* size of attribute value */
    __le32 e_hash;       /* hash value of name and value */
    char e_name[];       /* attribute name */
};

#define EXT4_XATTR_PAD_BITS 2
#define EXT4_XATTR_PAD (1 << EXT4_XATTR_PAD_BITS)
#define EXT4_XATTR_ROUND (EXT4_XATTR_PAD - 1)
#define EXT4_XATTR_LEN(name_len)                                                                   \
    (((name_len) + EXT4_XATTR_ROUND + sizeof(struct ext4_xattr_entry)) & ~EXT4_XATTR_ROUND)
#define EXT4_XATTR_NEXT(entry)                                                                     \
    ((struct ext4_xattr_entry *)((char *)(entry) + EXT4_XATTR_LEN((entry)->e_name_len)))
#define EXT4_XATTR_SIZE(size) (((size) + EXT4_XATTR_ROUND) & ~EXT4_XATTR_ROUND)

#define IHDR(inode, raw_inode)                                                                     \
    ((struct ext4_xattr_ibody_header *)((char *)raw_inode + EXT4_GOOD_OLD_INODE_SIZE +             \
                                        EXT4_I(inode)->i_extra_isize))
#define ITAIL(inode, raw_inode) ((char *)(raw_inode) + EXT4_SB((inode)->i_sb)->s_inode_size)
#define IFIRST(hdr) ((struct ext4_xattr_entry *)((hdr) + 1))

/*
 * XATTR_SIZE_MAX is currently 64k, but for the purposes of checking
 * for file system consistency errors, we use a somewhat bigger value.
 * This allows XATTR_SIZE_MAX to grow in the future, but by using this
 * instead of INT_MAX for certain consistency checks, we don't need to
 * worry about arithmetic overflows.  (Actually XATTR_SIZE_MAX is
 * defined in include/uapi/linux/limits.h, so changing it is going
 * not going to be trivial....)
 */
#define EXT4_XATTR_SIZE_MAX (1 << 24)

/*
 * The minimum size of EA value when you start storing it in an external inode
 * size of block - size of header - size of 1 entry - 4 null bytes
 */
#define EXT4_XATTR_MIN_LARGE_EA_SIZE(b)                                                            \
    ((b) - EXT4_XATTR_LEN(3) - sizeof(struct ext4_xattr_header) - 4)

#define BHDR(bh) ((struct ext4_xattr_header *)((bh)->b_data))
#define ENTRY(ptr) ((struct ext4_xattr_entry *)(ptr))
#define BFIRST(bh) ENTRY(BHDR(bh) + 1)
#define IS_LAST_ENTRY(entry) (*(__u32 *)(entry) == 0)

#define EXT4_ZERO_XATTR_VALUE ((void *)-1)

/*
 * If we want to add an xattr to the inode, we should make sure that
 * i_extra_isize is not 0 and that the inode size is not less than
 * EXT4_GOOD_OLD_INODE_SIZE + extra_isize + pad.
 *   EXT4_GOOD_OLD_INODE_SIZE   extra_isize header   entry   pad  data
 * |--------------------------|------------|------|---------|---|-------|
 */
#define EXT4_INODE_HAS_XATTR_SPACE(inode)                                                          \
    ((EXT4_I(inode)->i_extra_isize != 0) &&                                                        \
     (EXT4_GOOD_OLD_INODE_SIZE + EXT4_I(inode)->i_extra_isize +                                    \
          sizeof(struct ext4_xattr_ibody_header) + EXT4_XATTR_PAD <=                               \
      EXT4_INODE_SIZE((inode)->i_sb)))

struct ext4_xattr_info {
    const char *name;
    const void *value;
    size_t value_len;
    int name_index;
    int in_inode;
};

struct ext4_xattr_search {
    struct ext4_xattr_entry *first;
    void *base;
    void *end;
    struct ext4_xattr_entry *here;
    int not_found;
};

struct ext4_xattr_ibody_find {
    struct ext4_xattr_search s;
    struct ext4_iloc iloc;
};

int ext4_xattr_ibody_find(struct inode *, struct ext4_xattr_info *, struct ext4_xattr_ibody_find *);
int __ext4_xattr_check_block(struct inode *, struct buffer_head *, const char *, unsigned int);
int __xattr_check_inode(struct inode *, struct ext4_xattr_ibody_header *, void *, const char *,
                        unsigned int);
int ext4_xattr_get(struct inode *, int, const char *, void *, size_t);
int ext4_xattr_visit(struct inode *, ext_xattr_emit, void *);
int ext4_xattr_visit_block(struct inode *, ext_xattr_emit, void *, bool);
#define ext4_xattr_check_block(inode, bh) __ext4_xattr_check_block(inode, bh, __func__, __LINE__)
#define xattr_check_inode(inode, header, end)                                                      \
    __xattr_check_inode(inode, header, end, __func__, __LINE__)
#endif
