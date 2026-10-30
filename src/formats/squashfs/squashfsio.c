// SPDX-License-Identifier: GPL-2.0-or-later
#include "formats/squashfs/squashfsio.h"
#include "formats/squashfs/squashfs_fs_sb.h"
#include "formats/squashfs/squashfscompat.h"

// Replace block-device bio submission with a bounded archive source read.
int squashfs_bio_read(struct super_block *sb, u64 index, int length, struct bio **result,
                      int *offset) {
    struct sevenzip_squashfs_io *io = sb->s_fs_info->read_context;
    struct bio *bio;
    int error = -ENOMEM;
    if (length <= 0 || index > (u64)sb->s_fs_info->bytes_used ||
        (unsigned)length > (u64)sb->s_fs_info->bytes_used - index || !io->read)
        return -EIO;
    bio = calloc(1, sizeof(*bio));
    if (!bio)
        return -ENOMEM;
    bio->segment.data = malloc((size_t)length);
    if (!bio->segment.data)
        goto failed;
    bio->segment.bv_len = (unsigned)length;
    error = io->read(io->opaque, index, (void *)bio->segment.data, (size_t)length);
    if (error) {
        if (error > 0)
            error = -EIO;
        goto failed;
    }
    *result = bio;
    *offset = 0;
    return 0;
failed:
    bio_free_pages(bio);
    free(bio);
    return error;
}
