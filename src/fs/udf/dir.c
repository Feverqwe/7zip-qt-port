// SPDX-License-Identifier: GPL-2.0-only
/*
 * dir.c
 *
 * PURPOSE
 *  Directory handling routines for the OSTA-UDF(tm) filesystem.
 *
 * COPYRIGHT
 *  (C) 1998-2004 Ben Fennema
 *
 * HISTORY
 *
 *  10/05/98 dgb  Split directory operations into its own file
 *                Implemented directory reads via do_udf_readdir
 *  10/06/98      Made directory operations work!
 *  11/17/98      Rewrote directory to support ICBTAG_FLAG_AD_LONG
 *  11/25/98 blf  Rewrote directory handling (readdir+lookup) to support reading
 *                across blocks.
 *  12/12/98      Split out the lookup code to namei.c. bulk of directory
 *                code now in directory.c:udf_fileident_read.
 */

/* Archive-stream services for the original Linux file identifier iterator. */
#include "udfdecl.h"

struct sevenzip_udf_directory {
    struct super_block sb;
    struct inode inode;
    struct udf_fileident_iter iter;
    int result;
    bool advance;
};

void sevenzip_udf_directory_free(struct sevenzip_udf_directory *directory)
{
    if (directory) {
        udf_fiiter_release(&directory->iter);
        udf_clear_extent_cache(&directory->inode);
        free(directory);
    }
}

int sevenzip_udf_directory_open(void *context, sevenzip_udf_read read,
    uint32_t block_size, uint32_t partitions, const struct sevenzip_udf_file *file,
    struct sevenzip_udf_directory **output)
{
    struct sevenzip_udf_directory *directory;
    struct super_block *sb;
    struct inode *inode;
    int result;
    *output = NULL;
    if (file->size > INT64_MAX)
        return -EFSCORRUPTED;
    directory = calloc(1, sizeof(*directory));
    if (!directory)
        return -ENOMEM;
    sb = &directory->sb;
    inode = &directory->inode;
    sb->context = context;
    sb->read = read;
    sb->info.s_partitions = partitions;
    sb_set_blocksize(sb, block_size);
    inode->i_sb = sb;
    inode->i_size = file->size;
    inode->i_blkbits = sb->s_blocksize_bits;
    inode->info.i_data = (uint8_t *)(file->inline_data ? file->data : file->allocation_data);
    inode->info.i_lenAlloc = file->allocation_size;
    inode->info.i_alloc_type = file->allocation_type;
    inode->info.i_efe = file->extended;
    inode->info.i_location.logicalBlockNum = file->block;
    inode->info.i_location.partitionReferenceNum = file->partition;
    inode->info.cached_extent.lstart = -1;
    result = file->size ? udf_fiiter_init(&directory->iter, inode, 0) : 0;
    if (result) {
        sevenzip_udf_directory_free(directory);
        return result;
    }
    *output = directory;
    return 0;
}

int sevenzip_udf_directory_next(struct sevenzip_udf_directory *directory,
                                struct sevenzip_udf_entry *entry)
{
    struct udf_fileident_iter *iter = &directory->iter;
    if (directory->advance) {
        directory->advance = false;
        directory->result = udf_fiiter_advance(iter);
    }
    if (directory->result)
        return directory->result;
    if (iter->pos >= directory->inode.i_size)
        return 0;
    entry->flags = iter->fi.fileCharacteristics;
    entry->icb = &iter->fi.icb;
    entry->name = iter->name;
    entry->name_size = iter->fi.lengthFileIdent;
    directory->advance = true;
    return 1;
}
