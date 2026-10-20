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

int sevenzip_udf_directory(void *context, sevenzip_udf_read read, sevenzip_udf_dirent emit,
                          uint32_t block_size, uint32_t partitions,
                          const struct sevenzip_udf_file *file)
{
    struct super_block sb = {0};
    struct inode inode = {0};
    struct udf_fileident_iter iter = {0};
    int result;
    if (file->size > INT64_MAX)
        return -EFSCORRUPTED;
    if (!file->size)
        return 0;
    sb.context = context;
    sb.read = read;
    sb.info.s_partitions = partitions;
    sb_set_blocksize(&sb, block_size);
    inode.i_sb = &sb;
    inode.i_size = file->size;
    inode.i_blkbits = sb.s_blocksize_bits;
    inode.info.i_data = (uint8_t *)(file->inline_data ? file->data : file->allocation_data);
    inode.info.i_lenAlloc = file->allocation_size;
    inode.info.i_alloc_type = file->allocation_type;
    inode.info.i_efe = file->extended;
    inode.info.i_location.logicalBlockNum = file->block;
    inode.info.i_location.partitionReferenceNum = file->partition;
    inode.info.cached_extent.lstart = -1;
    for (result = udf_fiiter_init(&iter, &inode, 0);
         !result && iter.pos < inode.i_size;
         result = udf_fiiter_advance(&iter)) {
        if (emit(context, iter.fi.fileCharacteristics, &iter.fi.icb,
                 iter.name, iter.fi.lengthFileIdent)) {
            result = -EIO;
            break;
        }
    }
    udf_fiiter_release(&iter);
    udf_clear_extent_cache(&inode);
    return result;
}
