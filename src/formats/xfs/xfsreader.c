// SPDX-License-Identifier: GPL-3.0-or-later
// Local read-only byte-source boundary; all filesystem decoding is in GRUB.
#include "dependencies/grub/include/grub/fs/xfs_compat.h"

int grub_disk_read(grub_disk_t disk, uint64_t sector, uint64_t offset,
                   size_t size, void *buffer)
{
    if (disk->error)
        return disk->error;
    if (disk->source.poll && disk->source.poll(disk->source.context))
        return grub_error(GRUB_ERR_READ_ERROR, "cancelled");
    if (sector > (UINT64_MAX >> 9) || offset > UINT64_MAX - (sector << 9))
        return grub_error(GRUB_ERR_OUT_OF_RANGE, "source offset overflow");
    offset += sector << 9;
    if (offset > disk->source.size || size > disk->source.size - offset)
        return grub_error(GRUB_ERR_OUT_OF_RANGE, "read outside source");
    if (disk->source.read(disk->source.context, offset, buffer, size))
        return grub_error(GRUB_ERR_READ_ERROR, "source read failed");
    return 0;
}
