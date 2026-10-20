// SPDX-License-Identifier: GPL-2.0-only
// Linux unpack_to_rootfs/flush_buffer transitions around the authoritative SDK CPIO reader.
#include "initramfsreader.h"
#include "7zip/Common/LimitedStreams.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/MyCom.h"
#include "initramfscache.h"
#include "initramfsdecoders.h"
#include <algorithm>
#include <cstring>

namespace {
struct Progress {
    IArchiveOpenCallback *callback;
    UInt64 bytes = 0;
    HRESULT check(UInt64 next) {
        if (next > bytes)
            bytes = next;
        return callback ? callback->SetCompleted(nullptr, &bytes) : S_OK;
    }
};

class CheckedOutput final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    CMyComPtr<ISequentialOutStream> output;
    Progress *progress;
    UInt64 physicalPosition = 0, written = 0;
    bool checkPrefix = false, prefixDone = false;
    Byte prefix[6]{};
    unsigned prefixSize = 0;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (checkPrefix && !prefixDone) {
            const auto *p = static_cast<const Byte *>(data);
            for (UInt32 i = 0; i < size && prefixSize < sizeof(prefix); ++i)
                prefix[prefixSize++] = p[i];
            static const Byte start[] = {'0', '7', '0', '7', '0'};
            for (unsigned i = 0; i < std::min<unsigned>(prefixSize, sizeof(start)); ++i)
                if (prefix[i] != start[i])
                    return S_FALSE;
            if (prefixSize == sizeof(prefix)) {
                if (prefix[5] != '1' && prefix[5] != '2')
                    return S_FALSE;
                prefixDone = true;
            }
        }
        UInt32 done = 0;
        while (done < size) {
            RINOK(progress->check(physicalPosition))
            const UInt32 chunk = std::min<UInt32>(size - done, 1 << 15);
            UInt32 actual = 0;
            const HRESULT result =
                output->Write(static_cast<const Byte *>(data) + done, chunk, &actual);
            done += actual;
            written += actual;
            if (processed)
                *processed = done;
            RINOK(result)
            if (actual == 0)
                return E_FAIL;
        }
        return S_OK;
    }
};

class CheckedInput final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IInStream, ISequentialInStream)
  public:
    CMyComPtr<IInStream> source;
    Progress *progress;
    UInt64 position = 0;
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        RINOK(progress->check(position))
        UInt32 actual = 0;
        const HRESULT result = source->Read(data, std::min<UInt32>(size, 1 << 15), &actual);
        position += actual;
        if (processed)
            *processed = actual;
        return result;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *newPosition)) override {
        RINOK(source->Seek(offset, origin, &position))
        if (newPosition)
            *newPosition = position;
        return S_OK;
    }
};

HRESULT Peek(IInStream *stream, UInt64 position, Byte *bytes, size_t &size) {
    RINOK(InStream_SeekSet(stream, position))
    return ReadStream(stream, bytes, &size);
}

HRESULT SkipZeros(IInStream *stream, UInt64 end, UInt64 &position, Progress &progress,
                  bool physical = true) {
    Byte bytes[1 << 15];
    while (position < end) {
        RINOK(progress.check(physical ? position : progress.bytes))
        size_t size = size_t(std::min<UInt64>(sizeof(bytes), end - position));
        const size_t requested = size;
        RINOK(Peek(stream, position, bytes, size))
        size_t zeros = 0;
        while (zeros < size && bytes[zeros] == 0)
            ++zeros;
        position += zeros;
        if (zeros != size)
            return S_OK;
        if (size != requested)
            return S_FALSE;
    }
    return S_OK;
}

HRESULT CopyRange(IInStream *source, UInt64 offset, UInt64 size, CheckedOutput *output) {
    RINOK(InStream_SeekSet(source, offset))
    Byte bytes[1 << 15];
    while (size) {
        const UInt32 count = UInt32(std::min<UInt64>(sizeof(bytes), size));
        RINOK(ReadStream_FALSE(source, bytes, count))
        RINOK(WriteStream(output, bytes, count))
        size -= count;
        output->physicalPosition += count;
    }
    return S_OK;
}

HRESULT AlignCache(IInStream *cache, CheckedOutput *output, UInt64 &end) {
    RINOK(InStream_SeekSet(cache, end))
    const Byte zeros[3] = {};
    const unsigned padding = unsigned((UInt64(0) - end) & 3);
    RINOK(WriteStream(output, zeros, padding))
    end += padding;
    return S_OK;
}

HRESULT ValidateCompressed(IInStream *cache, UInt64 start, UInt64 size,
                           InitramfsReadRecord readRecord, bool &reset, bool &sawHeader,
                           Progress &progress) {
    CMyComPtr2_Create<IInStream, CExtentsStream> view;
    view->Stream = cache;
    CSeekExtent extent;
    extent.Virt = 0;
    extent.Phy = start;
    view->Extents.Add(extent);
    extent.Virt = size;
    view->Extents.Add(extent);
    view->Init();
    UInt64 position = 0;
    while (position < size) {
        Byte first;
        size_t count = 1;
        RINOK(Peek(view, position, &first, count))
        if (count != 1)
            return S_FALSE;
        if (first == 0) {
            // At initial Start, Linux interprets zero bytes as a bad header.
            // Only Reset (a completed previous record) permits this padding.
            if (!reset)
                return S_FALSE;
            RINOK(SkipZeros(view, size, position, progress, false))
            continue;
        }
        if ((position & 3) || first != '0')
            return S_FALSE;
        InitramfsRecord item;
        RINOK(readRecord(view, position, item))
        if (!item.isNew || item.dataPosition > size || item.storedSize > size - item.dataPosition ||
            (item.isTrailer && item.size))
            return S_FALSE;
        position = item.dataPosition + item.storedSize;
        reset = true;
        sawHeader = true;
    }
    // Unlike an uncompressed final member, a decompressor must leave Reset.
    return reset ? S_OK : S_FALSE;
}
} // namespace

HRESULT NormalizeInitramfs(IInStream *source, IArchiveOpenCallback *callback,
                           InitramfsReadRecord readRecord, IInStream **normalized,
                           UInt64 &physicalSize) {
    if (!source || !normalized || !readRecord)
        return E_INVALIDARG;
    *normalized = nullptr;
    RINOK(InStream_AtBegin_GetSize(source, physicalSize))
    if (physicalSize > INT64_MAX)
        return S_FALSE;
    if (callback)
        RINOK(callback->SetTotal(nullptr, &physicalSize))
    Progress progress{callback};
    CMyComPtr2_Create<IInStream, CheckedInput> input;
    input->source = source;
    input->progress = &progress;
    CMyComPtr<IInStream> cache;
    CMyComPtr<ISequentialOutStream> cacheOutput;
    CMyComPtr2_Create<ISequentialOutStream, CheckedOutput> output;
    output->progress = &progress;
    UInt64 position = 0, cacheEnd = 0;
    bool reset = false, sawHeader = false;
    while (position < physicalSize) {
        RINOK(progress.check(position))
        Byte bytes[2];
        size_t count = sizeof(bytes);
        RINOK(Peek(input, position, bytes, count))
        if (!count)
            return cache ? S_FALSE : S_OK;
        if (bytes[0] == 0) {
            RINOK(SkipZeros(input, physicalSize, position, progress))
            continue;
        }
        const InitramfsMethod method = InitramfsCompression(bytes, count);
        if (method != InitramfsMethod::None) {
            if (!cache) {
                RINOK(CreateInitramfsCache(&cache, &cacheOutput))
                output->output = cacheOutput;
                output->physicalPosition = 0;
                RINOK(CopyRange(input, 0, position, output.ClsPtr()))
                cacheEnd = position;
            }
            RINOK(AlignCache(cache, output.ClsPtr(), cacheEnd))
            const UInt64 start = cacheEnd;
            output->physicalPosition = position;
            output->written = 0;
            output->checkPrefix = !reset;
            output->prefixDone = false;
            output->prefixSize = 0;
            UInt64 consumed;
            RINOK(DecodeInitramfsFrame(method, input, position, physicalSize, output, nullptr,
                                       consumed))
            output->checkPrefix = false;
            cacheEnd += output->written;
            RINOK(ValidateCompressed(cache, start, output->written, readRecord, reset, sawHeader,
                                     progress))
            position += consumed;
            continue;
        }
        if (bytes[0] != '0' || (position & 3))
            return cache ? S_FALSE : S_OK;
        InitramfsRecord item;
        const HRESULT result = readRecord(input, position, item);
        if (result != S_OK)
            return result == S_FALSE && !cache ? S_OK : result;
        if (!item.isNew || item.dataPosition > physicalSize)
            return cache ? S_FALSE : S_OK;
        UInt64 stored = item.storedSize;
        if (stored > physicalSize - item.dataPosition) {
            if (item.size > physicalSize - item.dataPosition)
                return cache ? S_FALSE : S_OK;
            // Linux doesn't require final padding for the last raw member.
            stored = physicalSize - item.dataPosition;
        }
        if (item.isTrailer && item.size)
            return cache ? S_FALSE : S_OK;
        const UInt64 next = item.dataPosition + stored;
        if (cache) {
            RINOK(AlignCache(cache, output.ClsPtr(), cacheEnd))
            output->physicalPosition = position;
            RINOK(CopyRange(input, position, next - position, output.ClsPtr()))
            cacheEnd += next - position;
        }
        position = next;
        reset = true;
        sawHeader = true;
    }
    if (!cache)
        return S_OK;
    if (!sawHeader)
        return S_FALSE;
    RINOK(progress.check(physicalSize))
    RINOK(InStream_SeekSet(cache, 0))
    *normalized = cache.Detach();
    return S_OK;
}
