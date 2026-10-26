// (C) 2017 Tino Reichardt. LGPL-2.1-or-later.
// CDecoder / raw st_decompress adapted from 7-Zip-zstd. The compression
// algorithm remains the unmodified Google Brotli C implementation.
#include "BrotliDecoder.h"
#include "7zip/Common/StreamUtils.h"
#include <span>

#ifdef SEVENZIP_USE_BROTLI_MODULE
import sevenzip.dependencies.brotli;
#else
#include "dependencies/brotli.h"
#endif

namespace NCompress {
namespace NBROTLI {
Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
                            const UInt64 *, const UInt64 *, ICompressProgressInfo *progress)) {
    _processedIn = _processedOut = 0;
    NeedMoreInput = DataAfterEnd = false;
    sevenzip::brotli::Decoder state;
    using sevenzip::brotli::Status;
    if (!state)
        return E_OUTOFMEMORY;
    Byte input[1 << 15], output[1 << 15];
    std::span<const Byte> pendingInput;
    Status result = Status::needs_input;
    for (;;) {
        if (progress) {
            RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut))
        }
        if (result == Status::needs_input) {
            UInt32 size = 0;
            RINOK(inStream->Read(input, sizeof(input), &size))
            if (size == 0) {
                NeedMoreInput = true;
                return S_FALSE;
            }
            pendingInput = {input, size};
        }
        const auto step = state.decode(pendingInput, output);
        pendingInput = pendingInput.subspan(step.consumed);
        result = step.status;
        _processedIn += step.consumed;
        const size_t produced = step.produced;
        RINOK(WriteStream(outStream, output, produced))
        _processedOut += produced;
        if (result == Status::data_error || result == Status::memory_error) {
            if (result == Status::memory_error)
                return E_OUTOFMEMORY;
            return S_FALSE;
        }
        if (result == Status::done) {
            DataAfterEnd = !pendingInput.empty();
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
