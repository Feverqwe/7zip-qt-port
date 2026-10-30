// SPDX-License-Identifier: GPL-2.0-or-later
// Bounded filesystem stream services around the unchanged official zlib API.
#include "formats/squashfs/squashfszlib.h"
#include "7zip/Common/OutBuffer.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/ZlibDecoder.h"
#include "Common/MyBuffer.h"
#include "platform/cpu/CpuArch.h"
#include <algorithm>
#include <array>
#include <new>
#include <span>

#ifdef SEVENZIP_USE_ZLIB_MODULE
import sevenzip.dependencies.zlib;
#else
#include "dependencies/zlib/zlib.h"
#endif

namespace {
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

    sevenzip::zlib::Inflater decoder;
    using sevenzip::zlib::Status;
    if (decoder.status() != Status::ok)
        return decoder.status() == Status::memory_error ? E_OUTOFMEMORY : S_FALSE;
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
    std::span<const Byte> pendingInput;
    UInt32 produced = 0;
    auto checksum = sevenzip::zlib::initial_checksum();
    bool eof = false;
    for (;;) {
        if (pendingInput.empty() && !eof) {
            UInt32 count = 0;
            RINOK(input->Read(inputBuffer, UInt32(inputBuffer.Size()), &count))
            if (count > inputBuffer.Size())
                return E_FAIL;
            pendingInput = {static_cast<const Byte *>(inputBuffer), count};
            eof = count == 0;
        }
        const bool guard = produced == limit;
        const auto outputSize =
            guard ? 1
                  : std::min<UInt32>(limit - produced, static_cast<UInt32>(outputBuffer.size()));
        const auto step = decoder.decode(pendingInput, std::span(outputBuffer).first(outputSize));
        pendingInput = pendingInput.subspan(step.consumed);
        const auto written = static_cast<UInt32>(step.produced);
        if (guard && written)
            return S_FALSE;
        if (written) {
            buffered.value.WriteBytes(outputBuffer.data(), written);
            checksum =
                sevenzip::zlib::update_checksum(checksum, std::span(outputBuffer).first(written));
            produced += written;
        }
        if (step.status == Status::stream_end)
            break;
        if (step.status != Status::ok || (!step.consumed && !written && eof))
            return step.status == Status::memory_error ? E_OUTOFMEMORY : S_FALSE;
    }
    RINOK(buffered.Finish())
    Byte trailer[4];
    const auto available = std::min<std::size_t>(pendingInput.size(), sizeof(trailer));
    if (available)
        memcpy(trailer, pendingInput.data(), available);
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
