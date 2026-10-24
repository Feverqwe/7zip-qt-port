// SPDX-License-Identifier: GPL-2.0-or-later
// Bounded filesystem stream services around the unchanged official zlib API.
#include "squashfszlib.h"
#include "7zip/Common/OutBuffer.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/ZlibDecoder.h"
#include "C/CpuArch.h"
#include "Common/MyBuffer.h"
#include <algorithm>
#include <array>
#include <new>
#include <zlib.h>

namespace {
struct InflateState {
    z_stream value{};
    ~InflateState() {
        if (value.state)
            inflateEnd(&value);
    }
};

// Match the SDK's deferred flush: a decode/read error precedes a flush error,
// whereas a successful decode must flush before the Adler trailer is checked.
struct BufferedOutput {
    COutBuffer value;
    bool flushOnExit = false;
    ~BufferedOutput() {
        if (flushOnExit)
            value.Flush();
    }
    HRESULT Finish() {
        flushOnExit = false;
        return value.Flush();
    }
};

HRESULT Decode(ISequentialInStream *input, ISequentialOutStream *output, UInt32 limit) {
    Byte header[2];
    RINOK(ReadStream_FALSE(input, header, sizeof(header)))
    if (!NCompress::NZlib::IsZlib(header))
        return S_FALSE;

    InflateState decoder;
    const int init = inflateInit2(&decoder.value, -MAX_WBITS);
    if (init != Z_OK)
        return init == Z_MEM_ERROR ? E_OUTOFMEMORY : S_FALSE;
    // The SDK validates CMF/FLG separately and decodes with the full window.
    // Its input read-ahead and output window also determine I/O error priority.
    CByteBuffer inputBuffer(1 << 20);
    std::array<Byte, 1 << 15> outputBuffer;
    BufferedOutput buffered;
    if (!buffered.value.Create(1 << 15))
        return E_OUTOFMEMORY;
    buffered.value.SetStream(output);
    buffered.value.Init();
    buffered.flushOnExit = true;
    auto &state = decoder.value;
    UInt32 produced = 0;
    uLong checksum = adler32(0, Z_NULL, 0);
    bool eof = false;
    for (;;) {
        if (!state.avail_in && !eof) {
            UInt32 count = 0;
            RINOK(input->Read(inputBuffer, UInt32(inputBuffer.Size()), &count))
            if (count > inputBuffer.Size())
                return E_FAIL;
            state.next_in = inputBuffer;
            state.avail_in = count;
            eof = count == 0;
        }
        const bool guard = produced == limit;
        state.next_out = outputBuffer.data();
        state.avail_out = guard ? 1 : std::min<UInt32>(limit - produced, outputBuffer.size());
        const auto beforeIn = state.avail_in, beforeOut = state.avail_out;
        const int result = inflate(&state, Z_NO_FLUSH);
        const UInt32 written = beforeOut - state.avail_out;
        if (guard && written)
            return S_FALSE;
        if (written) {
            buffered.value.WriteBytes(outputBuffer.data(), written);
            checksum = adler32(checksum, outputBuffer.data(), written);
            produced += written;
        }
        if (result == Z_STREAM_END)
            break;
        if (result != Z_OK || (beforeIn == state.avail_in && !written && eof))
            return result == Z_MEM_ERROR ? E_OUTOFMEMORY : S_FALSE;
    }
    RINOK(buffered.Finish())
    Byte trailer[4];
    const UInt32 available = std::min<UInt32>(state.avail_in, sizeof(trailer));
    if (available)
        memcpy(trailer, state.next_in, available);
    RINOK(ReadStream_FALSE(input, trailer + available, sizeof(trailer) - available))
    return GetBe32(trailer) == checksum ? S_OK : S_FALSE;
}
} // namespace

HRESULT SquashfsDecodeZlib(ISequentialInStream *input, ISequentialOutStream *output,
                           UInt32 outputLimit) {
    try {
        return Decode(input, output, outputLimit);
    } catch (const CSystemException &error) {
        return error.ErrorCode;
    } catch (const std::bad_alloc &) {
        return E_OUTOFMEMORY;
    } catch (...) {
        return S_FALSE;
    }
}
