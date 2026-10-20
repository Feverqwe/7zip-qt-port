// SPDX-License-Identifier: GPL-2.0-only
// clang-format off
#include "StdAfx.h"
// clang-format on
#include "../../../../udfreader.h"
#include "../../../Common/UTFConvert.h"
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
    unsigned fileSet = 0;
    int recurse = 0;

    static int DirectoryEntry(void *opaque, Byte flags, const void *icb, const void *name,
                              Byte size) {
        auto &context = *static_cast<CLinuxContext *>(opaque);
        if (context.result != S_OK || context.progressResult != S_OK)
            return 1;
        // Default udf_readdir skips parent, deleted, hidden and invalid names.
        if (flags & (1 | 4 | 8))
            return 0;
        try {
            context.result = context.archive->ReadLinuxDirectoryFile(
                context.volume, context.fileSet, *context.item, static_cast<const Byte *>(icb),
                static_cast<const Byte *>(name), size, context.recurse);
        } catch (...) {
            context.result = E_OUTOFMEMORY;
        }
        return context.result != S_OK;
    }
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
    bool metadata = false;

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
            result = archive.ReadItem(builder.volume, builder.metadata ? -3 : -2, location, false,
                                      0, &builder.fatal);
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
                file->block = source->item.Location.Pos;
                file->allocation_type = source->item.IcbTag.GetDescriptorType();
                file->extended = source->item.IsExtended;
                file->allocation_size = (UInt32)source->item.LinuxAllocData.Size();
                file->allocation_data = source->item.LinuxAllocData;
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
    static int Status(void *opaque) { return (int)static_cast<CVatBuilder *>(opaque)->fatal; }
    static int ReadBlock(void *opaque, UInt16 partition, UInt32 block, void *data, size_t size) {
        auto &builder = *static_cast<CVatBuilder *>(opaque);
        if (builder.fatal != S_OK)
            return (int)builder.fatal;
        CLinuxContext context{builder.archive, builder.volume, nullptr};
        ReadLinuxBlock(&context, partition, block, data, size);
        if (context.progressResult != S_OK)
            builder.fatal = context.progressResult;
        if (context.result == E_ABORT)
            builder.fatal = E_ABORT;
        return (int)context.result;
    }
};

HRESULT CInArchive::LoadLinuxMetadata(unsigned volIndex, unsigned partitionRef) {
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
    auto context = std::make_shared<CVatBuilder>();
    context->archive = this;
    context->volume = volIndex;
    context->metadata = true;
    sevenzip_udf_metadata *state = nullptr;
    const int result = sevenzip_udf_metadata_create(
        context.get(), CVatBuilder::Open, CVatBuilder::ReadBlock, CVatBuilder::Close,
        CVatBuilder::Status, vol.BlockSize, vol.PartitionMaps.Size(), (UInt16)partitionRef,
        (UInt16)backing, map.MetadataMap, &state);
    std::shared_ptr<sevenzip_udf_metadata> owner(
        state, [context](sevenzip_udf_metadata *p) { sevenzip_udf_metadata_free(p); });
    if (context->fatal != S_OK)
        return context->fatal;
    if (result)
        return result < -4095 ? (HRESULT)result : result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
    map.Metadata = std::move(owner);
    CPartition partition = Partitions[map.PartitionIndex];
    partition.IsMetadata = true;
    map.PartitionIndex = Partitions.Add(partition);
    return S_OK;
}

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
        vol.PartitionMaps.Size(), blocks, _lastRecordedBlock, Partitions[map.PartitionIndex].Pos,
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
        if (c.result == S_OK && c.archive->_progress) {
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
        if (c.result == S_OK && c.archive->_progress) {
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
                                  CItem &item, bool hidden, HRESULT *progressResult) {
    CLinuxContext context{this, volIndex, nullptr};
    const bool previousUnexpectedEnd = UnexpectedEnd;
    const CLogVol &vol = LogVols[volIndex];
    CRecordVector<UInt32> lengths;
    FOR_VECTOR(i, vol.PartitionMaps)
    lengths.Add(Partitions[vol.PartitionMaps[i].PartitionIndex].Len);
    uint16_t partition = location.Location.PartitionRef;
    uint32_t block = location.Location.Pos;
    const int result =
        sevenzip_udf_icb(&context, ReadLinuxBlock, vol.BlockSize, &lengths[0], lengths.Size(),
                         &partition, &block, data, hidden, &item.LinuxAttributes);
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

HRESULT CInArchive::ReadLinuxFileset(unsigned volIndex, const CLongAllocDesc &location,
                                     CByteBuffer &data) {
    CLinuxContext context{this, volIndex, nullptr};
    const int result =
        sevenzip_udf_fileset(&context, ReadLinuxBlock, LogVols[volIndex].BlockSize,
                             location.Location.PartitionRef, location.Location.Pos, data);
    if (context.progressResult != S_OK)
        return context.progressResult;
    if (context.result != S_OK)
        return context.result;
    return result == 0 ? S_OK : result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
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
HRESULT CInArchive::ReadLinuxDirectory(unsigned volIndex, unsigned fsIndex, CItem &item,
                                       const Byte *data, UInt32 size, int numRecurseAllowed) {
    const CLogVol &vol = LogVols[volIndex];
    CLinuxContext context{this, volIndex, &item};
    context.fileSet = fsIndex;
    context.recurse = numRecurseAllowed;
    sevenzip_udf_file source{};
    source.size = item.Size;
    source.partition = (UInt16)item.Location.PartitionRef;
    source.block = item.Location.Pos;
    source.inline_data = item.IsInline;
    source.data = source.allocation_data = data;
    source.allocation_size = size;
    source.allocation_type = item.IcbTag.GetDescriptorType();
    source.extended = item.IsExtended;
    const int result =
        sevenzip_udf_directory(&context, ReadLinuxBlock, CLinuxContext::DirectoryEntry,
                               vol.BlockSize, vol.PartitionMaps.Size(), &source);
    if (context.progressResult != S_OK)
        return context.progressResult;
    if (context.result != S_OK)
        return context.result;
    return !result ? S_OK : result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
}

HRESULT CInArchive::ReadLinuxSymlink(unsigned volIndex, CItem &item) {
    const CLogVol &vol = LogVols[volIndex];
    // udf_symlink_filler accepts at most one filesystem block, and uses raw
    // udf_bread for external data rather than synthesizing holes at logical EOF.
    if (item.Size > vol.BlockSize)
        return S_FALSE;
    CByteBuffer raw;
    Byte *data;
    if (item.IsInline) {
        if (!item.CheckChunkSizes())
            return S_FALSE;
        data = item.InlineData;
    } else {
        if (item.Extents.IsEmpty() || !item.Extents[0].IsRecAndAlloc())
            return S_FALSE;
        raw.Alloc(vol.BlockSize);
        const CMyExtent &extent = item.Extents[0];
        RINOK(Read(volIndex, extent.PartitionRef, extent.Pos, vol.BlockSize, raw))
        data = raw;
    }
    Byte output[4096];
    if (sevenzip_udf_symlink(data, (int)item.Size, output, sizeof(output)))
        return S_FALSE;
    AString target((const char *)output);
    if (!ConvertUTF8ToUnicode(target, item.SymLink))
        return S_FALSE;
    item.Size = target.Len();
    item.IsInline = true;
    item.InlineData.CopyFrom((const Byte *)(const char *)target, target.Len());
    item.Extents.ClearAndFree();
    if (_progress)
        RINOK(_progress->SetCompleted(Files.Size(), _processedProgressBytes))
    return S_OK;
}
} // namespace NUdf
} // namespace NArchive
