// SPDX-License-Identifier: GPL-2.0-only
// Frame and record adapters around the original NTFS3 C readers.
#include "formats/ntfs/ntfsdecoder.h"
#include "formats/ntfs/ntfssecuritycompat.h"
int sevenzip_ntfs_mft_fixup(void *record, size_t size)
{
    int result;
    if (!record || size < 512 || size > 4096 || size % 512)
        return -EINVAL;
    result = ntfs_fix_post_read(record, size, false);
    return result == -E_NTFS_FIXUP ? 1 : result;
}

int sevenzip_ntfs_lznt(const void *input, size_t input_size, void *output, size_t output_size)
{
    ssize_t size;
    if (!input_size || input_size > 65536 || !output_size || output_size > 65536)
        return -EINVAL;
    size = decompress_lznt(input, input_size, output, output_size);
    if (size < 0) return (int)size;
    if (!size || (size_t)size > output_size) return -EINVAL;
    if ((size_t)size < output_size)
        memset((unsigned char *)output + size, 0, output_size - (size_t)size);
    return 0;
}
