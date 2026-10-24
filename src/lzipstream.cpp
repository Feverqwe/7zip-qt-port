// SPDX-License-Identifier: GPL-2.0-only
// Native stream services for the official C ffdecompress loop.
#include "lzipstream.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/Common/StreamUtils.h"
#include "lzipio.h"
#include <new>

namespace {
struct FrameIo {
    IInStream *source;
    ISequentialOutStream *output;
    ICompressProgressInfo *progress;
    HRESULT result = S_OK;

    template <typename Action> int invoke(Action action) noexcept {
        try {
            result = action();
        } catch (const std::bad_alloc &) {
            result = E_OUTOFMEMORY;
        } catch (...) {
            result = E_FAIL;
        }
        return result != S_OK;
    }
    static int read(void *opaque, void *data, size_t *size) noexcept {
        auto &io = *static_cast<FrameIo *>(opaque);
        return io.invoke([&] { return ReadStream(io.source, data, size); });
    }
    static int write(void *opaque, const void *data, size_t size) noexcept {
        auto &io = *static_cast<FrameIo *>(opaque);
        return io.invoke([&] { return WriteStream(io.output, data, size); });
    }
    static int checkpoint(void *opaque, uint64_t consumed, uint64_t produced) noexcept {
        auto &io = *static_cast<FrameIo *>(opaque);
        const UInt64 inputSize = consumed, outputSize = produced;
        return io.invoke([&] {
            return io.progress ? io.progress->SetRatioInfo(&inputSize, &outputSize) : S_OK;
        });
    }
};
} // namespace

HRESULT SevenzipDecodeLzip(IInStream *source, UInt64 offset, UInt64 sourceSize,
                           ISequentialOutStream *output, ICompressProgressInfo *progress,
                           UInt64 &consumed, UInt64 &produced, Int32 &operationResult) {
    using namespace NArchive::NExtract::NOperationResult;
    consumed = 0;
    operationResult = kDataError;
    if (!source || !output || offset > sourceSize || sourceSize > INT64_MAX)
        return E_INVALIDARG;
    RINOK(InStream_SeekSet(source, offset))
    FrameIo state{source, output, progress};
    sevenzip_lzip_io io{&state, FrameIo::read, FrameIo::write, FrameIo::checkpoint};
    uint64_t inputSize = 0, outputSize = produced;
    const int result = SevenzipLzipDecompress(&io, offset, sourceSize, &inputSize, &outputSize);
    consumed = inputSize;
    produced = outputSize;
    if (state.result != S_OK)
        return state.result;
    if (result == SEVENZIP_LZIP_MEMORY_ERROR)
        return E_OUTOFMEMORY;
    if (result == SEVENZIP_LZIP_IO_ERROR)
        return E_FAIL;
    if (result == SEVENZIP_LZIP_OK)
        operationResult = kOK;
    else if (result == SEVENZIP_LZIP_UNEXPECTED_END)
        operationResult = kUnexpectedEnd;
    else if (result == SEVENZIP_LZIP_TRAILING_DATA)
        operationResult = kDataAfterEnd;
    return S_OK;
}
