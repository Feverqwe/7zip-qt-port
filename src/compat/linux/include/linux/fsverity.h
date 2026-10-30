/* SPDX-License-Identifier: GPL-2.0 */
/*
 * fs-verity: read-only file-based authenticity protection
 *
 * This header declares the interface between the fs/verity/ support layer and
 * filesystems that support fs-verity.
 *
 * Copyright 2019 Google LLC
 */

#ifndef SEVENZIP_LINUX_FSVERITY_H
#define SEVENZIP_LINUX_FSVERITY_H
#define FS_VERITY_MAX_DIGEST_SIZE SHA512_DIGEST_SIZE
#define FS_VERITY_MAX_DESCRIPTOR_SIZE 16384
struct fsverity_operations {
    /**
     * Begin enabling verity on the given file.
     *
     * @filp: a readonly file descriptor for the file
     *
     * The filesystem must do any needed filesystem-specific preparations
     * for enabling verity, e.g. evicting inline data.  It also must return
     * -EBUSY if verity is already being enabled on the given file.
     *
     * i_rwsem is held for write.
     *
     * Return: 0 on success, -errno on failure
     */
    int (*begin_enable_verity)(struct file *filp);

    /**
     * End enabling verity on the given file.
     *
     * @filp: a readonly file descriptor for the file
     * @desc: the verity descriptor to write, or NULL on failure
     * @desc_size: size of verity descriptor, or 0 on failure
     * @merkle_tree_size: total bytes the Merkle tree took up
     *
     * If desc == NULL, then enabling verity failed and the filesystem only
     * must do any necessary cleanups.  Else, it must also store the given
     * verity descriptor to a fs-specific location associated with the inode
     * and do any fs-specific actions needed to mark the inode as a verity
     * inode, e.g. setting a bit in the on-disk inode.  The filesystem is
     * also responsible for setting the S_VERITY flag in the VFS inode.
     *
     * i_rwsem is held for write, but it may have been dropped between
     * ->begin_enable_verity() and ->end_enable_verity().
     *
     * Return: 0 on success, -errno on failure
     */
    int (*end_enable_verity)(struct file *filp, const void *desc, size_t desc_size,
                             u64 merkle_tree_size);

    /**
     * Get the verity descriptor of the given inode.
     *
     * @inode: an inode with the S_VERITY flag set
     * @buf: buffer in which to place the verity descriptor
     * @bufsize: size of @buf, or 0 to retrieve the size only
     *
     * If bufsize == 0, then the size of the verity descriptor is returned.
     * Otherwise the verity descriptor is written to 'buf' and its actual
     * size is returned; -ERANGE is returned if it's too large.  This may be
     * called by multiple processes concurrently on the same inode.
     *
     * Return: the size on success, -errno on failure
     */
    int (*get_verity_descriptor)(struct inode *inode, void *buf, size_t bufsize);

    /**
     * Read a Merkle tree page of the given inode.
     *
     * @inode: the inode
     * @index: 0-based index of the page within the Merkle tree
     *
     * This can be called at any time on an open verity file.  It may be
     * called by multiple processes concurrently, even with the same page.
     *
     * Note that this must retrieve a *page*, not necessarily a *block*.
     *
     * Return: the page on success, ERR_PTR() on failure
     */
    struct page *(*read_merkle_tree_page)(struct inode *inode, pgoff_t index);

    /**
     * Perform readahead of a Merkle tree for the given inode.
     *
     * @inode: the inode
     * @index: 0-based index of the first page within the Merkle tree
     * @nr_pages: number of pages to be read ahead.
     *
     * This can be called at any time on an open verity file.  It may be
     * called by multiple processes concurrently, even with the same range.
     *
     * Optional method so that ->read_merkle_tree_page preferably finds
     * cached data instead of issuing dependent I/O.
     */
    void (*readahead_merkle_tree)(struct inode *inode, pgoff_t index, unsigned long nr_pages);

    /**
     * Write a Merkle tree block to the given file.
     *
     * @file: the file for which the Merkle tree is being built
     * @buf: the Merkle tree block to write
     * @pos: the position of the block in the Merkle tree (in bytes)
     * @size: the Merkle tree block size (in bytes)
     *
     * This is only called between ->begin_enable_verity() and
     * ->end_enable_verity().
     *
     * Return: 0 on success, -errno on failure
     */
    int (*write_merkle_tree_block)(struct file *file, const void *buf, u64 pos, unsigned int size);
};
#endif
