// SPDX-License-Identifier: GPL-2.0-only
// clang-format off
#include "StdAfx.h"
// clang-format on
#include "../../Common/RegisterArc.h"
#include "../../Common/StreamUtils.h"
#include "UdfIn.h"
#include <errno.h>

namespace NArchive {
namespace NUdf {
struct CInArchive::CDiscoveryContext {
    CInArchive *archive;
    HRESULT fatal = S_OK;
    HRESULT ioFailure = S_OK;
    static int Read(void *opaque, UInt16 partition, UInt32 block, void *data, size_t size) {
        auto &c = *static_cast<CDiscoveryContext *>(opaque);
        if (c.fatal != S_OK)
            return (int)c.fatal;
        const UInt64 offset = (UInt64)block * size;
        if (partition || offset > c.archive->FileSize || size > c.archive->FileSize - offset)
            return (int)S_FALSE;
        HRESULT result = E_FAIL;
        try {
            result = InStream_SeekSet(c.archive->_stream, offset);
            if (result == S_OK)
                result = ReadStream_FALSE(c.archive->_stream, data, size);
            if (result == S_OK && c.archive->_progress) {
                c.fatal = E_FAIL;
                c.fatal = c.archive->_progress->SetCompleted(0, c.archive->_processedProgressBytes);
                result = c.fatal;
            }
        } catch (...) {
            result = E_FAIL;
        }
        if (result == E_ABORT)
            c.fatal = result;
        if (FAILED(result) && c.ioFailure == S_OK)
            c.ioFailure = result;
        return (int)result;
    }
    static int Descriptor(void *opaque, UInt16 id, UInt32 block, const void *data, size_t size) {
        auto &c = *static_cast<CDiscoveryContext *>(opaque);
        if (c.fatal != S_OK)
            return (int)c.fatal;
        try {
            unsigned bits = 0;
            while (((size_t)1 << bits) < size)
                ++bits;
            c.archive->SecLogSize = bits;
            const HRESULT result =
                c.archive->ParseVolumeDescriptor(static_cast<const Byte *>(data), size, id);
            if (result == S_OK)
                c.archive->UpdatePhySize(((UInt64)block + 1) * size);
            if (result == E_OUTOFMEMORY || result == E_ABORT)
                c.fatal = result;
            return (int)result;
        } catch (...) {
            c.fatal = E_OUTOFMEMORY;
            return (int)c.fatal;
        }
    }
    static void Reset(void *opaque) {
        auto &archive = *static_cast<CDiscoveryContext *>(opaque)->archive;
        archive.PrimeVols.Clear();
        archive.LogVols.Clear();
        archive.Partitions.Clear();
        archive.PhySize = 0;
    }
    static int Status(void *opaque) { return (int)static_cast<CDiscoveryContext *>(opaque)->fatal; }
};

HRESULT CInArchive::LoadLinuxDiscovery() {
    CDiscoveryContext context{this};
    sevenzip_udf_discovery discovery{};
    const int result = sevenzip_udf_discover(
        &context, CDiscoveryContext::Read, CDiscoveryContext::Descriptor, CDiscoveryContext::Reset,
        CDiscoveryContext::Status, FileSize, &discovery);
    IsArc = discovery.seen_anchor != 0;
    if (context.fatal != S_OK)
        return context.fatal;
    if (result)
        return context.ioFailure != S_OK ? context.ioFailure
               : result == -ENOMEM       ? E_OUTOFMEMORY
                                         : S_FALSE;
    if (LogVols.IsEmpty() || LogVols[0].PartitionMaps.IsEmpty() || Partitions.IsEmpty())
        return S_FALSE;
    _lastRecordedBlock = discovery.last_block;
    UpdatePhySize(((UInt64)discovery.anchor_block + 1) * discovery.block_size);
    return S_OK;
}
} // namespace NUdf
} // namespace NArchive
