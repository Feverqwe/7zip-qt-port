/*
  LZ4io.c - LZ4 File/Stream Interface
  Copyright (C) Yann Collet 2011-2024

  GPL v2 License

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

  You can contact the author at :
  - LZ4 source repository : https://github.com/lz4/lz4
  - LZ4 public forum : https://groups.google.com/forum/#!forum/lz4c
*/
// SPDX-License-Identifier: GPL-2.0-only
// Bounded native-stream adaptation; provenance in docs/lz4-frame-baseline.json.
#include "formats/lz4/lz4frameio.h"
#include <lz4frame.h>

int LZ4IO_decompressLZ4F(struct sevenzip_lz4_frame_io *io, uint64_t offset,
                         uint64_t source_size, uint64_t *consumed, uint64_t *produced)
{
    unsigned char input[1 << 16], decoded[1 << 16];
    LZ4F_dctx *context = NULL;
    LZ4F_frameInfo_t info;
    size_t header_size, required, used, nextToLoad;
    int result = SEVENZIP_LZ4_DATA_ERROR;
    *consumed = 0;
    if (LZ4F_isError(LZ4F_createDecompressionContext(&context, LZ4F_VERSION)))
        return SEVENZIP_LZ4_MEMORY_ERROR;

    /* Parse a complete header with the library and restore the consumed boundary. */
    header_size = source_size - offset < LZ4F_HEADER_SIZE_MAX
        ? (size_t)(source_size - offset) : LZ4F_HEADER_SIZE_MAX;
    if (io->read(io->opaque, input, &header_size))
        goto io_error;
    if (header_size < LZ4F_MIN_SIZE_TO_KNOW_HEADER_LENGTH)
        goto unexpected_end;
    required = LZ4F_headerSize(input, header_size);
    if (LZ4F_isError(required))
        goto cleanup;
    if (required > header_size)
        goto unexpected_end;
    used = header_size;
    nextToLoad = LZ4F_getFrameInfo(context, &info, input, &used);
    if (LZ4F_isError(nextToLoad))
        goto cleanup;
    *consumed = used;
    if (io->seek(io->opaque, offset + *consumed))
        goto io_error;

    /* Main Loop: retain the upstream hint, bounded feed and full-output flush. */
    for (; nextToLoad;) {
        size_t readSize;
        size_t pos = 0;
        size_t decodedBytes = sizeof(decoded);
        if (io->progress(io->opaque, offset + *consumed, *produced))
            goto io_error;
        if (nextToLoad > sizeof(input))
            nextToLoad = sizeof(input);
        if (nextToLoad > source_size - offset - *consumed)
            nextToLoad = (size_t)(source_size - offset - *consumed);
        readSize = nextToLoad;
        if (io->read(io->opaque, input, &readSize))
            goto io_error;
        if (!readSize)
            goto unexpected_end;
        while ((pos < readSize) || (decodedBytes == sizeof(decoded))) {
            size_t remaining = readSize - pos;
            decodedBytes = sizeof(decoded);
            nextToLoad = LZ4F_decompress(context, decoded, &decodedBytes,
                                         input + pos, &remaining, NULL);
            if (LZ4F_isError(nextToLoad))
                goto cleanup;
            pos += remaining;
            *consumed += remaining;
            if (*produced > UINT64_MAX - decodedBytes)
                goto cleanup;
            if (io->write(io->opaque, decoded, decodedBytes))
                goto io_error;
            *produced += decodedBytes;
            if (io->progress(io->opaque, offset + *consumed, *produced))
                goto io_error;
            if (!nextToLoad)
                break;
            if (!remaining && !decodedBytes) {
                if (pos == readSize)
                    break;
                goto cleanup;
            }
        }
    }
    if (io->seek(io->opaque, offset + *consumed))
        goto io_error;
    result = SEVENZIP_LZ4_OK;
    goto cleanup;
unexpected_end:
    result = SEVENZIP_LZ4_UNEXPECTED_END;
    goto cleanup;
io_error:
    result = SEVENZIP_LZ4_IO_ERROR;
cleanup:
    LZ4F_freeDecompressionContext(context);
    return result;
}
