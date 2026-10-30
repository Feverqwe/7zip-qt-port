// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2016-present, Facebook, Inc. All rights reserved.
 * Linux zstd_decompress/decompress_bio, using the existing 7-Zip decoder
 * in place of kernel workspace/dstream and folio services. */
#include "C/Alloc.h"
#include "C/ZstdDec.h"
#include "formats/btrfs/compression.h"

int zstd_decompress(const void *input, size_t srclen, void *output, size_t destlen,
                    int inline_extent) {
    CZstdDecHandle stream = ZstdDec_Create(&g_AlignedAlloc, &g_BigAlloc);
    if (!stream)
        return -ENOMEM;
    uint8_t *buf = malloc(BTRFS_MAX_UNCOMPRESSED);
    if (!buf) {
        ZstdDec_Destroy(stream);
        return -ENOMEM;
    }
    CZstdDecState state;
    ZstdDecState_Clear(&state);
    state.inBuf = input;
    state.inLim = srclen;
    state.outBuf_fromCaller = buf;
    state.outBufSize_fromCaller = BTRFS_MAX_UNCOMPRESSED;
    state.outSize_Defined = True;
    state.outSize = BTRFS_MAX_UNCOMPRESSED;
    ZstdDec_Init(stream);
    SRes ret;
    for (;;) {
        const size_t before_in = state.inPos, before_out = state.winPos;
        ret = ZstdDec_Decode(stream, &state);
        state.wrPos = state.winPos;
        if (ret != SZ_OK || state.info.windowSize_MAX > BTRFS_MAX_UNCOMPRESSED ||
            state.status == ZSTD_STATUS_FINISHED_FRAME || state.inPos == srclen)
            break;
        if (before_in == state.inPos && before_out == state.winPos) {
            ret = SZ_ERROR_DATA;
            break;
        }
    }
    /* A single frame ends before the extent's on-disk sector padding. */
    int error = ret == SZ_ERROR_MEM ? -ENOMEM : -EILSEQ;
    if (ret == SZ_OK && state.info.windowSize_MAX <= BTRFS_MAX_UNCOMPRESSED &&
        state.info.num_DataFrames == 1 && !state.info.num_SkipFrames && !state.info.dictionaryId &&
        state.status == ZSTD_STATUS_FINISHED_FRAME &&
        (!inline_extent || state.outProcessed >= destlen) &&
        state.outProcessed <= BTRFS_MAX_UNCOMPRESSED) {
        memcpy(output, buf, state.outProcessed < destlen ? (size_t)state.outProcessed : destlen);
        error = 0;
    }
    free(buf);
    ZstdDec_Destroy(stream);
    return error;
}
