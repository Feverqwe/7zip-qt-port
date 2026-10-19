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

HRESULT CInArchive::ReadLinuxItem(unsigned volIndex, CLongAllocDesc &location, CByteBuffer &data) {
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
    if (context.progressResult != S_OK)
        return context.progressResult;
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
                                     const Byte *data, UInt32 size) {
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
    if (context.result != S_OK)
        return context.result;
    return result == 0 ? S_OK : S_FALSE;
}
} // namespace NUdf
} // namespace NArchive
