// SPDX-License-Identifier: GPL-2.0-only
// Read-frame normalization from ntfs3/frecord.c::ni_read_frame.
#include "ntfsdecoder.h"
#include "fs/ntfs3/ntfscompat.h"
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
