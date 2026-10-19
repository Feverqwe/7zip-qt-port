// SPDX-License-Identifier: GPL-2.0-only
// clang-format off
#include "StdAfx.h"
// clang-format on
#include "../../../../udfreader.h"
#include "../../Common/RegisterArc.h"
#include "../../Common/StreamUtils.h"
#include "UdfIn.h"
#include <errno.h>

namespace NArchive {
namespace NUdf {
struct CInArchive::CLinuxContext {
    CInArchive *archive;
    unsigned volume;
    CItem *item;
    HRESULT result = S_OK;
    HRESULT progressResult = S_OK;
};

struct CInArchive::CVatSource {
    CInArchive *archive;
    unsigned volume;
    CItem item;
    CMyComPtr<IInStream> reader;
    bool creating = false;

    HRESULT Read(UInt32 block, void *buffer, size_t length) {
        if (creating)
            return S_FALSE;
        if (!reader) {
            // Linux udf_bread does not synthesize VAT cells from unallocated extents.
            if (!item.IsInline) {
                if (item.GetChunksSumSize() < item.Size)
                    return S_FALSE;
                FOR_VECTOR(i, item.Extents)
                if (!item.Extents[i].IsRecAndAlloc())
                    return S_FALSE;
            }
            creating = true;
            CMyComPtr<ISequentialInStream> stream;
            const HRESULT result = archive->GetItemStream(volume, item, &stream);
            creating = false;
            RINOK(result)
            RINOK(stream->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&reader)))
            archive = nullptr;
        }
        const UInt64 offset = (UInt64)block * length;
        if (offset >= item.Size)
            return S_FALSE;
        memset(buffer, 0, length);
        RINOK(InStream_SeekSet(reader, offset))
        return ReadStream_FALSE(reader, buffer, (size_t)MyMin((UInt64)length, item.Size - offset));
    }
};

struct CInArchive::CVatBuilder {
    CInArchive *archive;
    unsigned volume;
    HRESULT fatal = S_OK;

    static int Open(void *opaque, UInt16 partition, UInt32 block, sevenzip_udf_file *file) {
        auto &builder = *static_cast<CVatBuilder *>(opaque);
        if (builder.fatal != S_OK)
            return (int)builder.fatal;
        auto &archive = *builder.archive;
        const unsigned count = archive.Items.Size();
        const UInt32 extents = archive._numExtents;
        const UInt64 inlineSize = archive._inlineExtentsSize;
        HRESULT result = E_FAIL;
        try {
            CLongAllocDesc location;
            location.Len = archive.LogVols[builder.volume].BlockSize;
            location.Location.PartitionRef = partition;
            location.Location.Pos = block;
            result = archive.ReadItem(builder.volume, -2, location, false, 0, &builder.fatal);
            if (result == S_OK && archive.Items.Back().IsInline &&
                !archive.Items.Back().CheckChunkSizes())
                result = S_FALSE;
            if (result == S_OK) {
                std::unique_ptr<CVatSource> source(new CVatSource);
                source->archive = &archive;
                source->volume = builder.volume;
                source->item = archive.Items.Back();
                file->size = source->item.Size;
                file->partition = source->item.Location.PartitionRef;
                file->inline_data = source->item.IsInline;
                file->data =
                    source->item.IsInline ? (const Byte *)source->item.InlineData : nullptr;
                file->handle = source.release();
            }
        } catch (...) {
            result = E_OUTOFMEMORY;
        }
        while (archive.Items.Size() > count)
            archive.Items.DeleteBack();
        archive._numExtents = extents;
        archive._inlineExtentsSize = inlineSize;
        if (result == E_ABORT || result == E_OUTOFMEMORY)
            builder.fatal = result;
        return (int)result;
    }

    static int Read(void *opaque, UInt32 block, void *buffer, size_t length) {
        try {
            return (int)static_cast<CVatSource *>(opaque)->Read(block, buffer, length);
        } catch (...) {
            return (int)E_OUTOFMEMORY;
        }
    }
    static void Close(void *opaque) { delete static_cast<CVatSource *>(opaque); }
};

HRESULT CInArchive::LoadLinuxVat(unsigned volIndex, unsigned partitionRef) {
    const CLogVol &vol = LogVols[volIndex];
    CPartitionMap &map = LogVols[volIndex].PartitionMaps[partitionRef];
    unsigned backing = vol.PartitionMaps.Size();
    FOR_VECTOR(i, vol.PartitionMaps) {
        const CPartitionMap &candidate = vol.PartitionMaps[i];
        if (candidate.PartitionNumber == map.PartitionNumber &&
            (candidate.Type == 1 || candidate.IsSparable())) {
            backing = i;
            break;
        }
    }
    if (backing == vol.PartitionMaps.Size())
        return S_FALSE;
    const UInt64 blocks = FileSize / vol.BlockSize;
    if (!blocks || blocks > (UInt64)UINT32_MAX + 1)
        return S_FALSE;
    CVatBuilder builder{this, volIndex};
    sevenzip_udf_virtual *state = nullptr;
    const int result = sevenzip_udf_virtual_create(
        &builder, CVatBuilder::Open, CVatBuilder::Read, CVatBuilder::Close, vol.BlockSize,
        vol.PartitionMaps.Size(), blocks, (UInt32)(blocks - 1), Partitions[map.PartitionIndex].Pos,
        (UInt16)partitionRef, (UInt16)backing, map.IsVat20, &state);
    std::shared_ptr<sevenzip_udf_virtual> owner(state, sevenzip_udf_virtual_free);
    if (builder.fatal != S_OK)
        return builder.fatal;
    if (result < -4095)
        return (HRESULT)result;
    if (result)
        return result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
    map.Virtual = std::move(owner);
    return S_OK;
}

int CInArchive::ReadLinuxPhysical(void *opaque, UInt16, UInt32 block, void *buffer, size_t length) {
    auto &c = *static_cast<CLinuxContext *>(opaque);
    if (c.progressResult != S_OK || c.result == E_ABORT)
        return -1;
    try {
        const UInt64 offset = (UInt64)block << c.archive->SecLogSize;
        if (offset > c.archive->FileSize || length > c.archive->FileSize - offset) {
            c.result = S_FALSE;
            return -1;
        }
        c.result = InStream_SeekSet(c.archive->_stream, offset);
        if (c.result == S_OK)
            c.result = ReadStream_FALSE(c.archive->_stream, buffer, length);
        if (c.result == S_OK) {
            c.archive->UpdatePhySize(offset + length);
            c.progressResult = E_FAIL;
            c.progressResult = c.archive->_progress->SetCompleted(
                c.archive->Files.Size(), c.archive->_processedProgressBytes);
            c.result = c.progressResult;
        }
    } catch (...) {
        c.result = E_FAIL;
    }
    return c.result == S_OK ? 0 : -1;
}

HRESULT CInArchive::LoadLinuxSparing(unsigned volIndex, unsigned partitionRef) {
    CPartitionMap &map = LogVols[volIndex].PartitionMaps[partitionRef];
    CLinuxContext context{this, volIndex, nullptr};
    sevenzip_udf_sparing *state = nullptr;
    const int result =
        sevenzip_udf_sparing_create(&context, ReadLinuxPhysical, LogVols[volIndex].BlockSize,
                                    Partitions[map.PartitionIndex].Pos, map.SparableMap, &state);
    std::shared_ptr<sevenzip_udf_sparing> owner(state, sevenzip_udf_sparing_free);
    if (context.progressResult != S_OK)
        return context.progressResult;
    if (context.result == E_ABORT)
        return E_ABORT;
    if (result != 0)
        return result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
    map.Sparing = std::move(owner);
    return S_OK;
}

int CInArchive::ReadLinuxBlock(void *opaque, UInt16 partition, UInt32 block, void *buffer,
                               size_t length) {
    auto &c = *static_cast<CLinuxContext *>(opaque);
    try {
        c.result = c.archive->Read(c.volume, partition, block, (UInt32)length,
                                   static_cast<Byte *>(buffer));
        if (c.result == S_OK) {
            c.progressResult = E_FAIL;
            c.progressResult = c.archive->_progress->SetCompleted(
                c.archive->Files.Size(), c.archive->_processedProgressBytes);
            c.result = c.progressResult;
        }
    } catch (...) {
        c.result = E_FAIL;
    }
    return c.result == S_OK ? 0 : -1;
}

HRESULT CInArchive::ReadLinuxItem(unsigned volIndex, CLongAllocDesc &location, CByteBuffer &data,
                                  HRESULT *progressResult) {
    CLinuxContext context{this, volIndex, nullptr};
    const bool previousUnexpectedEnd = UnexpectedEnd;
    const CLogVol &vol = LogVols[volIndex];
    CRecordVector<UInt32> lengths;
    FOR_VECTOR(i, vol.PartitionMaps)
    lengths.Add(Partitions[vol.PartitionMaps[i].PartitionIndex].Len);
    uint16_t partition = location.Location.PartitionRef;
    uint32_t block = location.Location.Pos;
    const int result = sevenzip_udf_icb(&context, ReadLinuxBlock, vol.BlockSize, &lengths[0],
                                        lengths.Size(), &partition, &block, data);
    if (context.progressResult != S_OK) {
        if (progressResult)
            *progressResult = context.progressResult;
        return context.progressResult;
    }
    if (context.result == E_ABORT)
        return E_ABORT;
    if (result != 0)
        return context.result != S_OK ? context.result
               : result == -ENOMEM    ? E_OUTOFMEMORY
                                      : S_FALSE;
    // Linux deliberately retains the previous FE when its optional IE cannot be read.
    UnexpectedEnd = previousUnexpectedEnd;
    if (PhySize > FileSize)
        PhySize = FileSize;
    location.Location.PartitionRef = partition;
    location.Location.Pos = block;
    return S_OK;
}

HRESULT CInArchive::ReadLinuxExtents(unsigned volIndex, unsigned partitionRef, CItem &item,
                                     const Byte *data, UInt32 size, HRESULT *progressResult) {
    CLinuxContext context{this, volIndex, &item};
    const auto emit = [](void *opaque, unsigned type, uint32_t length, uint32_t block,
                         uint16_t partition) -> int {
        auto &c = *static_cast<CLinuxContext *>(opaque);
        try {
            CMyExtent extent;
            extent.Len = length | ((UInt32)type << 30);
            extent.Pos = block;
            extent.PartitionRef = partition;
            c.item->Extents.Add(extent);
        } catch (...) {
            c.result = E_OUTOFMEMORY;
        }
        return c.result == S_OK ? 0 : -1;
    };
    const CLogVol &vol = LogVols[volIndex];
    const int result = sevenzip_udf_extents(&context, ReadLinuxBlock, emit, vol.BlockSize,
                                            vol.PartitionMaps.Size(), (uint16_t)partitionRef,
                                            item.IcbTag.GetDescriptorType(), item.IsExtended, data,
                                            size, item.Size, ((UInt32)1 << 30) - _numExtents);
    if (context.progressResult != S_OK && progressResult)
        *progressResult = context.progressResult;
    if (context.result != S_OK)
        return context.result;
    return result == 0 ? S_OK : S_FALSE;
}
} // namespace NUdf
} // namespace NArchive
