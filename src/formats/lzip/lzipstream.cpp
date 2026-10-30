// SPDX-License-Identifier: GPL-2.0-only
// Native stream services for the official C ffdecompress loop.
#include "formats/lzip/lzipstream.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/Common/StreamUtils.h"
#include <new>
#include <span>

#ifdef SEVENZIP_USE_LZLIB_MODULE
import sevenzip.dependencies.lzlib;
#else
#include "dependencies/lzlib/lzlib.h"
#endif

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
    int read(std::span<std::byte> data, size_t &size) noexcept {
        return invoke([&] { return ReadStream(source, data.data(), &size); });
    }
    int write(std::span<const std::byte> data) noexcept {
        return invoke([&] { return WriteStream(output, data.data(), data.size()); });
    }
    int checkpoint(uint64_t consumed, uint64_t produced) noexcept {
        const UInt64 inputSize = consumed, outputSize = produced;
        return invoke(
            [&] { return progress ? progress->SetRatioInfo(&inputSize, &outputSize) : S_OK; });
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
    const auto result =
        sevenzip::lzlib::decode(state, {.offset = offset, .size = sourceSize}, produced);
    using sevenzip::lzlib::Status;
    consumed = result.consumed;
    produced = result.produced;
    if (state.result != S_OK)
        return state.result;
    if (result.status == Status::memory_error)
        return E_OUTOFMEMORY;
    if (result.status == Status::io_error)
        return E_FAIL;
    if (result.status == Status::ok)
        operationResult = kOK;
    else if (result.status == Status::unexpected_end)
        operationResult = kUnexpectedEnd;
    else if (result.status == Status::trailing_data)
        operationResult = kDataAfterEnd;
    return S_OK;
}
