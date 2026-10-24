/* File-to-file decompression loop adapted from lzlib ffexample.c.
   Copyright (C) 2010-2026 Antonio Diaz Diaz.
   Unlimited permission to copy, distribute, and modify this example.
   See docs/lzip-baseline.json. Decoding remains unchanged in lzlib. */
#include "lzipio.h"
#include <lzlib.h>

int SevenzipLzipDecompress(struct sevenzip_lzip_io *io, uint64_t offset,
                          uint64_t source_size, uint64_t *consumed, uint64_t *produced)
{
    uint8_t buffer[16384];
    uint64_t fed = 0;
    int result = SEVENZIP_LZIP_DATA_ERROR;
    int member_seen = 0;
    LZ_Decoder *decoder = LZ_decompress_open();
    *consumed = 0;
    if (!decoder || LZ_decompress_errno(decoder) != LZ_ok) {
        result = SEVENZIP_LZIP_MEMORY_ERROR;
        goto cleanup;
    }
    for (;;) {
        int ret;
        int size = LZ_decompress_write_size(decoder);
        if (size > (int)sizeof(buffer)) size = sizeof(buffer);
        if (size > 0) {
            size_t len = source_size - offset - fed < (uint64_t)size
                ? (size_t)(source_size - offset - fed) : (size_t)size;
            if (io->read(io->opaque, buffer, &len)) goto io_error;
            ret = LZ_decompress_write(decoder, buffer, (int)len);
            if (ret < 0) break;
            if (ret != (int)len) goto cleanup;
            fed += len;
            if (!len || fed == source_size - offset) LZ_decompress_finish(decoder);
        }
        ret = LZ_decompress_read(decoder, buffer, sizeof(buffer));
        *consumed = LZ_decompress_total_in_size(decoder);
        if (ret < 0) break;
        if (*produced > UINT64_MAX - (unsigned)ret) goto cleanup;
        if (io->write(io->opaque, buffer, (size_t)ret)) goto io_error;
        *produced += ret;
        if (io->progress(io->opaque, offset + *consumed, *produced)) goto io_error;
        if (LZ_decompress_errno(decoder) == LZ_ok &&
            LZ_decompress_member_finished(decoder) == 1) member_seen = 1;
        if (LZ_decompress_finished(decoder) == 1) {
            result = SEVENZIP_LZIP_OK;
            goto cleanup;
        }
    }
    switch (LZ_decompress_errno(decoder)) {
    case LZ_mem_error: result = SEVENZIP_LZIP_MEMORY_ERROR; break;
    case LZ_unexpected_eof: result = SEVENZIP_LZIP_UNEXPECTED_END; break;
    case LZ_header_error:
        if (member_seen) result = SEVENZIP_LZIP_TRAILING_DATA;
        break;
    default: break;
    }
    goto cleanup;
io_error:
    result = SEVENZIP_LZIP_IO_ERROR;
cleanup:
    LZ_decompress_close(decoder);
    return result;
}
