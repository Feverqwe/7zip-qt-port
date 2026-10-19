// SPDX-License-Identifier: GPL-2.0-only
// clang-format off
#include "StdAfx.h"
// clang-format on
#include "../../../../udfreader.h"
#include "../../Common/RegisterArc.h"
#include "UdfIn.h"

namespace NArchive {
namespace NUdf {
HRESULT CInArchive::ReadLinuxExtents(unsigned volIndex, unsigned partitionRef, CItem &item,
                                     const Byte *data, UInt32 size) {
    struct Context {
        CInArchive *archive;
        unsigned volume;
        CItem *item;
        HRESULT result;
    } context{this, volIndex, &item, S_OK};
    const auto read = [](void *opaque, uint16_t partition, uint32_t block, void *buffer,
                         size_t length) -> int {
        auto &c = *static_cast<Context *>(opaque);
        try {
            c.result = c.archive->Read(c.volume, partition, block, (UInt32)length,
                                       static_cast<Byte *>(buffer));
            if (c.result == S_OK)
                c.result = c.archive->_progress->SetCompleted(c.archive->Files.Size(),
                                                              c.archive->_processedProgressBytes);
        } catch (...) {
            c.result = E_FAIL;
        }
        return c.result == S_OK ? 0 : -1;
    };
    const auto emit = [](void *opaque, unsigned type, uint32_t length, uint32_t block,
                         uint16_t partition) -> int {
        auto &c = *static_cast<Context *>(opaque);
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
    const int result = sevenzip_udf_extents(&context, read, emit, vol.BlockSize,
                                            vol.PartitionMaps.Size(), (uint16_t)partitionRef,
                                            item.IcbTag.GetDescriptorType(), item.IsExtended, data,
                                            size, item.Size, ((UInt32)1 << 30) - _numExtents);
    if (context.result != S_OK)
        return context.result;
    return result == 0 ? S_OK : S_FALSE;
}
} // namespace NUdf
} // namespace NArchive
