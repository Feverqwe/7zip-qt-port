// (C) 2017 Tino Reichardt. LGPL-2.1-or-later.
// CDecoder / raw st_decompress adapted from 7-Zip-zstd. The compression
// algorithm remains the unmodified Google Brotli C implementation.
#include "BrotliDecoder.h"
#include "7zip/Common/StreamUtils.h"
#include <brotli/decode.h>
#include <memory>

namespace NCompress {
namespace NBROTLI {
Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
                            const UInt64 *, const UInt64 *, ICompressProgressInfo *progress)) {
    _processedIn = _processedOut = 0;
    NeedMoreInput = DataAfterEnd = false;
    std::unique_ptr<BrotliDecoderState, decltype(&BrotliDecoderDestroyInstance)> state(
        BrotliDecoderCreateInstance(nullptr, nullptr, nullptr), BrotliDecoderDestroyInstance);
    if (!state)
        return E_OUTOFMEMORY;
    Byte input[1 << 15], output[1 << 15];
    size_t availableIn = 0;
    const uint8_t *nextIn = input;
    BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
    for (;;) {
        if (progress) {
            RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut))
        }
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            UInt32 size = 0;
            RINOK(inStream->Read(input, sizeof(input), &size))
            if (size == 0) {
                NeedMoreInput = true;
                return S_FALSE;
            }
            availableIn = size;
            nextIn = input;
        }
        uint8_t *nextOut = output;
        size_t availableOut = sizeof(output);
        const size_t before = availableIn;
        result = BrotliDecoderDecompressStream(state.get(), &availableIn, &nextIn, &availableOut,
                                               &nextOut, nullptr);
        _processedIn += before - availableIn;
        const size_t produced = sizeof(output) - availableOut;
        RINOK(WriteStream(outStream, output, produced))
        _processedOut += produced;
        if (result == BROTLI_DECODER_RESULT_ERROR) {
            const auto error = BrotliDecoderGetErrorCode(state.get());
            if (error <= BROTLI_DECODER_ERROR_ALLOC_CONTEXT_MODES &&
                error >= BROTLI_DECODER_ERROR_ALLOC_BLOCK_TYPE_TREES)
                return E_OUTOFMEMORY;
            return S_FALSE;
        }
        if (result == BROTLI_DECODER_RESULT_SUCCESS) {
            DataAfterEnd = availableIn != 0;
            if (!DataAfterEnd) {
                UInt32 size = 0;
                RINOK(inStream->Read(input, 1, &size))
                DataAfterEnd = size != 0;
            }
            if (progress) {
                RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut))
            }
            return S_OK;
        }
    }
}
} // namespace NBROTLI
} // namespace NCompress
