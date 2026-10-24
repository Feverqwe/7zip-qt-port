// SPDX-License-Identifier: GPL-2.0-only
// Contiguous-buffer calls to the official LZ4, liblzma and zlib decoders.
#include "erofsreader.h"
#include <lz4.h>
#include <lzma.h>
#include <zlib.h>
#include <errno.h>
#include <limits.h>
#include "../third_party/7zip/C/Alloc.h"
#include "../third_party/7zip/C/ZstdDec.h"
#include <string.h>

static int erofs_decode_deflate(const void *input, size_t input_size, void *output,
                                size_t output_size, int partial) {
    z_stream state = {0};
    int status = inflateInit2(&state, -MAX_WBITS);
    if (status != Z_OK)
        return status == Z_MEM_ERROR ? -ENOMEM : -EILSEQ;
    state.next_in = (Bytef *)input;
    state.avail_in = (uInt)input_size;
    state.next_out = output;
    state.avail_out = (uInt)output_size;
    status = inflate(&state, Z_SYNC_FLUSH);
    if (!partial && status == Z_OK && !state.avail_out) {
        // Consume the end marker even when the caller's buffer is exactly full;
        // a further output byte means the extent's declared length is wrong.
        Bytef guard;
        state.next_out = &guard;
        state.avail_out = 1;
        status = inflate(&state, Z_SYNC_FLUSH);
    }
    int error = status == Z_MEM_ERROR ? -ENOMEM : -EILSEQ;
    if (state.total_out == output_size &&
        (status == Z_STREAM_END || (partial && status == Z_OK)))
        error = 0;
    inflateEnd(&state);
    return error;
}

static int erofs_decode_zstd(unsigned int dictionary, const void *input, size_t input_size,
                              void *output, size_t output_size, int partial) {
    if (dictionary < 1024 || dictionary > (1U << 20) || output_size > (12U << 20))
        return -EINVAL;
    CZstdDecHandle decoder = ZstdDec_Create(&g_AlignedAlloc, &g_BigAlloc);
    if (!decoder)
        return -ENOMEM;
    const size_t capacity = partial ? (12U << 20) : output_size;
    Byte *buffer = partial ? MidAlloc(capacity) : output;
    if (!buffer) {
        ZstdDec_Destroy(decoder);
        return -ENOMEM;
    }
    CZstdDecState state;
    ZstdDecState_Clear(&state);
    state.inBuf = input;
    state.inLim = input_size;
    state.outBuf_fromCaller = buffer;
    state.outBufSize_fromCaller = capacity;
    state.outSize_Defined = True;
    state.outSize = output_size;
    ZstdDec_Init(decoder);
    SRes result;
    for (;;) {
        size_t before_in = state.inPos, before_out = state.winPos;
        result = ZstdDec_Decode(decoder, &state);
        state.wrPos = state.winPos;
        if (result != SZ_OK || state.info.windowSize_MAX > dictionary ||
            (partial && state.outProcessed >= output_size) ||
            (state.inPos == state.inLim && ZstdDecState_DOES_NEED_MORE_INPUT_OR_FINISHED_FRAME(&state)))
            break;
        if (state.inPos == before_in && state.winPos == before_out) {
            result = SZ_ERROR_DATA;
            break;
        }
    }
    CZstdDecResInfo info;
    ZstdDec_GetResInfo(decoder, &state, result, &info);
    int error = result == SZ_ERROR_MEM ? -ENOMEM : -EILSEQ;
    if (result == SZ_OK && state.info.windowSize_MAX <= dictionary &&
        state.outProcessed >= output_size &&
        (partial || (state.outProcessed == output_size && info.decode_SRes == SZ_OK &&
                     !info.is_NonFinishedFrame))) {
        if (partial)
            memcpy(output, buffer, output_size);
        error = 0;
    }
    if (partial)
        MidFree(buffer);
    ZstdDec_Destroy(decoder);
    return error;
}

int erofs_decode_native(unsigned int algorithm, unsigned int dictionary, const void *input,
                         size_t input_size, void *output, size_t output_size, int partial) {
    if (input_size > INT_MAX || output_size > INT_MAX || !output_size)
        return -EINVAL;
    if (algorithm == 0) {
        int size = partial
            ? LZ4_decompress_safe_partial(input, output, (int)input_size, (int)output_size,
                                           (int)output_size)
            : LZ4_decompress_safe(input, output, (int)input_size, (int)output_size);
        return size == (int)output_size ? 0 : -EILSEQ;
    }
    if (algorithm == 1) {
        if (dictionary < 4096 || dictionary > (8U << 20))
            return -EINVAL;
        lzma_stream state = LZMA_STREAM_INIT;
        lzma_ret status = lzma_microlzma_decoder(&state, input_size, output_size,
                                               !partial, dictionary);
        if (status == LZMA_OK) {
            state.next_in = input;
            state.avail_in = input_size;
            state.next_out = output;
            state.avail_out = output_size;
            status = lzma_code(&state, LZMA_FINISH);
        }
        int error = status == LZMA_MEM_ERROR ? -ENOMEM : -EILSEQ;
        if (status == LZMA_STREAM_END && state.total_out == output_size)
            error = 0;
        lzma_end(&state);
        return error;
    }
    if (algorithm == 3)
        return erofs_decode_zstd(dictionary, input, input_size, output, output_size, partial);
    if (algorithm == 2)
        return erofs_decode_deflate(input, input_size, output, output_size, partial);
    return -EINVAL;
}
