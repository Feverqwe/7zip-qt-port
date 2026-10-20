// SPDX-License-Identifier: GPL-2.0-only
// Bounded 7-Zip streams around the original Linux C boot wrappers.
#include "linuxframestream.h"
#include "7zip/Common/StreamUtils.h"
#include "linuxframe.h"
#include <algorithm>
#include <cstring>
#include <new>

namespace {
struct FrameIo {
    IInStream *source;
    ISequentialOutStream *output;
    ICompressProgressInfo *progress;
    UInt64 position, end, produced;
    HRESULT result = S_OK;

    HRESULT checkpoint() {
        if (result == S_OK && progress)
            result = progress->SetRatioInfo(&position, &produced);
        return result;
    }
    static long fill(void *opaque, void *data, unsigned long size) noexcept {
        auto &io = *static_cast<FrameIo *>(opaque);
        unsigned long done = 0;
        try {
            while (done < size) {
                if (io.checkpoint() != S_OK)
                    return 0;
                const size_t request = size_t(std::min<UInt64>(
                    std::min<unsigned long>(size - done, 1 << 15), io.end - io.position));
                if (!request)
                    break;
                size_t actual = request;
                io.result = ReadStream(io.source, static_cast<Byte *>(data) + done, &actual);
                io.position += actual;
                done += static_cast<unsigned long>(actual);
                if (io.result != S_OK)
                    return 0;
                if (actual != request)
                    break;
            }
        } catch (const std::bad_alloc &) {
            io.result = E_OUTOFMEMORY;
            return 0;
        } catch (...) {
            io.result = E_FAIL;
            return 0;
        }
        return static_cast<long>(done);
    }
    static long flush(void *opaque, void *data, unsigned long size) noexcept {
        auto &io = *static_cast<FrameIo *>(opaque);
        unsigned long done = 0;
        try {
            while (done < size) {
                if (io.checkpoint() != S_OK)
                    return -1;
                const unsigned long chunk = std::min<unsigned long>(size - done, 1 << 15);
                if (io.produced > UINT64_MAX - chunk) {
                    io.result = E_INVALIDARG;
                    return -1;
                }
                io.result = WriteStream(io.output, static_cast<Byte *>(data) + done, chunk);
                if (io.result != S_OK)
                    return -1;
                done += chunk;
                io.produced += chunk;
            }
        } catch (const std::bad_alloc &) {
            io.result = E_OUTOFMEMORY;
            return -1;
        } catch (...) {
            io.result = E_FAIL;
            return -1;
        }
        return static_cast<long>(done);
    }
    static void error(void *, const char *) noexcept {}
};
} // namespace

HRESULT SevenzipDecodeLinuxFrame(bool lzo, IInStream *source, UInt64 offset, UInt64 sourceSize,
                                 ISequentialOutStream *output, ICompressProgressInfo *progress,
                                 UInt64 &consumed, UInt64 &produced) {
    consumed = 0;
    if (!source || !output || offset > sourceSize || sourceSize > INT64_MAX)
        return E_INVALIDARG;
    RINOK(InStream_SeekSet(source, offset))
    FrameIo state{source, output, progress, offset, sourceSize, produced};
    sevenzip_linux_frame_io io{&state, FrameIo::fill, FrameIo::flush, FrameIo::error, 0};
    const int result = lzo ? unlzo(&io, &consumed) : unlz4(&io, &consumed);
    produced = state.produced;
    if (state.result != S_OK)
        return state.result;
    if (io.allocation_failed)
        return E_OUTOFMEMORY;
    if (result || !consumed || consumed > sourceSize - offset)
        return S_FALSE;
    // LZO's initial header read can include following frames. The kernel's
    // consumed count, rather than callback read-ahead, defines the boundary.
    RINOK(InStream_SeekSet(source, offset + consumed))
    return state.checkpoint();
}
