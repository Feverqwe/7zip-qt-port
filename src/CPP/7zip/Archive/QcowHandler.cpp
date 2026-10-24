// QcowHandler.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "../../../C/Alloc.h"
#include "../../../C/CpuArch.h"
#include "../../../C/ZstdDec.h"

#include "../../Common/ComTry.h"
#include "../../Common/IntToString.h"
#include "../../Common/MyBuffer2.h"
#include "../../Common/UTFConvert.h"

#include "../../Windows/PropVariant.h"
#include "../../Windows/PropVariantUtils.h"
#include "../../Windows/TimeUtils.h"

#include "../Common/CWrappers.h"
#include "../Common/LimitedStreams.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "../Compress/DeflateDecoder.h"

#include "../../../block/qcow.h"
#include "../../../block/qcow2.h"
#include "HandlerCont.h"

#define Get32(p) GetBe32a(p)
#define Get64(p) GetBe64a(p)

using namespace NWindows;

namespace NArchive {
namespace NQcow {

static const Byte k_Signature[] = {'Q', 'F', 'I', 0xFB, 0, 0, 0};

/*
VA to PA maps:
  high bits (L1) :              : index in L1 (_dir) : _dir[high_index] points to Table.
  mid bits  (L2) : _numMidBits  : index in Table, Table[index] points to cluster start offset in arc
file. low bits       : _clusterBits : offset inside cluster.
*/

Z7_class_CHandler_final : public CHandlerImg {
    Z7_IFACE_COM7_IMP(IInArchive_Img)
    Z7_IFACE_COM7_IMP(IInArchiveGetStream)
    Z7_IFACE_COM7_IMP(ISequentialInStream)
    Z7_COM7F_IMP(GetNumberOfItems(UInt32 * numItems))
    Z7_COM7F_IMP(Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode,
                         IArchiveExtractCallback *extractCallback))

    unsigned _clusterBits;
    unsigned _numMidBits;
    unsigned _l2EntryBits;
    bool _extendedL2;
    UInt64 _compressedFlag;

    CObjArray2<UInt32> _dir;
    CAlignedBuffer _table;
    CByteBuffer _cache;
    CByteBuffer _cacheCompressed;
    UInt64 _cacheCluster;

    UInt64 _comprPos;
    size_t _comprSize;

    bool _needCompression;
    bool _isArc;
    bool _unsupported;
    Byte _compressionType;

    UInt64 _phySize;

    CMyComPtr2<ISequentialInStream, CBufInStream> _bufInStream;
    CMyComPtr2<ISequentialOutStream, CBufPtrSeqOutStream> _bufOutStream;
    CMyComPtr2<ICompressCoder, NCompress::NDeflate::NDecoder::CCOMCoder> _deflateDecoder;
    CZstdDecHandle _zstd;

    UInt32 _version;
    UInt32 _cryptMethod;
    UInt64 _incompatFlags;
    Qcow2HeaderExtensions _extensions{};
    QCowSnapshot *_snapshots = NULL;
    UInt32 _numSnapshots = 0;
    CMyComPtr<IInStream> _sourceStream;
    bool _isDiskStream = false;
    bool _useSnapshot = false;
    UInt64 _snapshotL1Offset = 0;
    UInt32 _snapshotL1Size = 0;
    UInt64 _snapshotDiskSize = 0;
    CMyComPtr<IInStream> _backingStream;
    CMyComPtr<IInStream> _dataStream;
    UInt64 _backingSize = 0;
    UInt64 _dataSize = 0;
    bool _externalData = false;
    UString _backingName;
    UString _sourceName;
    UString _errorMessage;
    unsigned _level = 0;
    CHandler *_child = NULL;

    HRESULT Seek2(UInt64 offset) {
        _posInArc = offset;
        return InStream_SeekSet(Stream, offset);
    }

    HRESULT InitAndSeek() {
        _virtPos = 0;
        return Seek2(0);
    }

    HRESULT Open2(IInStream * stream, IArchiveOpenCallback * openCallback) Z7_override;
    HRESULT OpenReference(IArchiveOpenCallback * openCallback, const UString &name,
                          CMyComPtr<IInStream> &input, UString &resolvedName);
    HRESULT OpenParent(IArchiveOpenCallback * openCallback);
    HRESULT CreateImage(UInt32 index, CHandler * &image, CMyComPtr<IInStream> & owner,
                        IArchiveOpenCallback *callback = NULL);

  public:
    CHandler() : _zstd(NULL) {}
    ~CHandler() {
        qcow2_free_extensions(&_extensions);
        qcow2_free_snapshots(_snapshots, _numSnapshots);
        if (_zstd)
            ZstdDec_Destroy(_zstd);
    }
};

static const UInt32 kEmptyDirItem = (UInt32)0 - 1;

Z7_COM7F_IMF(CHandler::Read(void *data, UInt32 size, UInt32 *processedSize)) {
    if (processedSize)
        *processedSize = 0;
    // printf("\nRead _virtPos = %6d  size = %6d\n", (UInt32)_virtPos, size);
    if (_virtPos >= _size)
        return S_OK;
    {
        const UInt64 rem = _size - _virtPos;
        if (size > rem)
            size = (UInt32)rem;
        if (size == 0)
            return S_OK;
    }

    for (;;) {
        bool inherit = true;
        const UInt64 cluster = _virtPos >> _clusterBits;
        const size_t clusterSize = (size_t)1 << _clusterBits;
        const size_t lowBits = (size_t)_virtPos & (clusterSize - 1);
        {
            const size_t rem = clusterSize - lowBits;
            if (size > rem)
                size = (UInt32)rem;
        }
        if (cluster == _cacheCluster) {
            memcpy(data, _cache + lowBits, size);
            break;
        }

        const UInt64 high = cluster >> _numMidBits;

        if (high < _dir.Size()) {
            const UInt32 tabl = _dir[(size_t)high];
            if (tabl != kEmptyDirItem) {
                const size_t midBits = (size_t)cluster & (((size_t)1 << _numMidBits) - 1);
                const Byte *p =
                    _table + ((((size_t)tabl << _numMidBits) + midBits) << _l2EntryBits);
                UInt64 v = Get64(p);

                if (v || _extendedL2) {
                    if (v & _compressedFlag) {
                        /*
                        the example of table record for 12-bit clusters (4KB uncompressed):
                          2 bits : isCompressed status
                          (4 == _clusterBits - 8) bits : (num_sectors - 1)
                              packSize = num_sectors * 512;
                              it uses one additional bit over unpacked cluster_bits.
                          (49 == 61 - _clusterBits) bits : offset of 512-byte sector
                          9 bits : offset in 512-byte sector
                        */
                        UInt64 sectorOffset;
                        size_t dataSize;
                        size_t offsetInSector = 0;
                        if (_version == 1) {
                            // QCOW1 stores an exact byte count and byte offset.
                            sectorOffset = qcow_get_compressed_cluster(v, _clusterBits, &dataSize);
                        } else {
                            const unsigned numOffsetBits = 62 - (_clusterBits - 8);
                            const UInt64 offset = v & (((UInt64)1 << 62) - 1);
                            dataSize = ((size_t)(offset >> numOffsetBits) + 1) << 9;
                            sectorOffset = offset & (((UInt64)1 << numOffsetBits) - (1 << 9));
                            offsetInSector = (size_t)offset & ((1 << 9) - 1);
                        }
                        const UInt64 offset2inCache = sectorOffset - _comprPos;

                        // QCOW2 _comprPos is aligned for 512-bytes; QCOW1 is byte-aligned.
                        // we try to use previous _cacheCompressed that contains compressed data
                        // that was read for previous unpacking

                        if (sectorOffset >= _comprPos && offset2inCache < _comprSize) {
                            if (offset2inCache) {
                                _comprSize -= (size_t)offset2inCache;
                                memmove(_cacheCompressed, _cacheCompressed + (size_t)offset2inCache,
                                        _comprSize);
                                _comprPos = sectorOffset;
                            }
                            sectorOffset += _comprSize;
                        } else {
                            _comprPos = sectorOffset;
                            _comprSize = 0;
                        }

                        if (dataSize > _comprSize) {
                            if (sectorOffset != _posInArc) {
                                // printf("\nDeflate-Seek %12I64x %12I64x\n", sectorOffset,
                                // sectorOffset - _posInArc);
                                RINOK(Seek2(sectorOffset))
                            }
                            if (_cacheCompressed.Size() < dataSize)
                                return E_FAIL;
                            const size_t dataSize3 = dataSize - _comprSize;
                            size_t dataSize2 = dataSize3;
                            // printf("\n\n=======\nReadStream = %6d _comprPos = %6d \n",
                            // (UInt32)dataSize2, (UInt32)_comprPos);
                            const HRESULT hres =
                                ReadStream(Stream, _cacheCompressed + _comprSize, &dataSize2);
                            _posInArc += dataSize2;
                            RINOK(hres)
                            if (dataSize2 != dataSize3)
                                return E_FAIL;
                            _comprSize += dataSize2;
                        }

                        _bufInStream->Init(_cacheCompressed + offsetInSector,
                                           dataSize - offsetInSector);
                        _cacheCluster = (UInt64)(Int64)-1;
                        if (_cache.Size() < clusterSize)
                            return E_FAIL;
                        _bufOutStream->Init(_cache, clusterSize);
                        // Do we need to use smaller block than clusterSize for last cluster?
                        const UInt64 blockSize64 = clusterSize;
                        HRESULT res;
                        if (_compressionType == 1) {
                            // SquashfsHandler's bounded caller-buffer Zstandard decode.
                            // QCOW stores one frame followed by sector padding, so stop at
                            // its frame boundary rather than decoding the padding.
                            CZstdDecState state;
                            ZstdDecState_Clear(&state);
                            state.inBuf = _cacheCompressed + offsetInSector;
                            state.inLim = dataSize - offsetInSector;
                            state.outBuf_fromCaller = _cache;
                            state.outBufSize_fromCaller = clusterSize;
                            ZstdDec_Init(_zstd);
                            SRes sres;
                            for (;;) {
                                sres = ZstdDec_Decode(_zstd, &state);
                                if (sres != SZ_OK || state.status == ZSTD_STATUS_FINISHED_FRAME ||
                                    state.status == ZSTD_STATUS_NEEDS_MORE_INPUT ||
                                    state.status == ZSTD_STATUS_OUT_REACHED)
                                    break;
                            }
                            CZstdDecResInfo info;
                            ZstdDec_GetResInfo(_zstd, &state, sres, &info);
                            res = SResToHRESULT(info.decode_SRes);
                            if (res == S_OK && (state.status != ZSTD_STATUS_FINISHED_FRAME ||
                                                state.winPos != clusterSize || info.extraSize != 0))
                                res = S_FALSE;
                        } else
                            res = _deflateDecoder.Interface()->Code(_bufInStream, _bufOutStream,
                                                                    NULL, &blockSize64, NULL);
                        /*
                        if (_bufOutStreamSpec->GetPos() != clusterSize)
                          memset(_cache + _bufOutStreamSpec->GetPos(), 0, clusterSize -
                        _bufOutStreamSpec->GetPos());
                        */
                        if (res == S_OK && _compressionType == 0)
                            if ((_version >= 2 && !_deflateDecoder->IsFinished()) ||
                                _bufOutStream->GetPos() != clusterSize)
                                res = S_FALSE;
                        RINOK(res)
                        _cacheCluster = cluster;
                        continue;
                        /*
                        memcpy(data, _cache + lowBits, size);
                        break;
                        */
                    }

                    // QEMU classifies extended entries per subcluster. Never expose
                    // stale host bytes from an unallocated or explicitly zero range.
                    bool allocated = ((UInt32)v & 511) != 1;
                    if (_version >= 2) {
                        unsigned subcluster = 0;
                        if (_extendedL2) {
                            const size_t subclusterSize = clusterSize >> 5;
                            subcluster = (unsigned)(lowBits / subclusterSize);
                            const size_t rem = subclusterSize - (lowBits & (subclusterSize - 1));
                            if (size > rem)
                                size = (UInt32)rem;
                        }
                        const QCow2SubclusterType type =
                            qcow2_get_subcluster_type(_extendedL2, _externalData, v,
                                                      _extendedL2 ? Get64(p + 8) : 0, subcluster);
                        if (type == QCOW2_SUBCLUSTER_INVALID)
                            return S_FALSE;
                        allocated = type == QCOW2_SUBCLUSTER_NORMAL;
                        inherit = type == QCOW2_SUBCLUSTER_UNALLOCATED_PLAIN ||
                                  type == QCOW2_SUBCLUSTER_UNALLOCATED_ALLOC;
                    } else {
                        inherit = v == 0;
                    }
                    if (allocated) {
                        v &= _compressedFlag - 1;
                        v += lowBits;
                        if (_externalData) {
                            RINOK(InStream_SeekSet(_dataStream, v))
                        } else if (v != _posInArc) {
                            // printf("\n%12I64x\n", v - _posInArc);
                            RINOK(Seek2(v))
                        }
                        IInStream *input =
                            _externalData ? _dataStream.Interface() : Stream.Interface();
                        const HRESULT res = input->Read(data, size, &size);
                        if (!_externalData)
                            _posInArc += size;
                        _virtPos += size;
                        if (processedSize)
                            *processedSize = size;
                        return res;
                    }
                }
            }
        }

        if (inherit && _backingStream && _virtPos < _backingSize) {
            const UInt64 rem = _backingSize - _virtPos;
            if (size > rem)
                size = (UInt32)rem;
            RINOK(InStream_SeekSet(_backingStream, _virtPos))
            size_t actual = size;
            const HRESULT result = ReadStream(_backingStream, data, &actual);
            RINOK(result)
            if (actual != size)
                return S_FALSE;
            break;
        }
        memset(data, 0, size);
        break;
    }

    _virtPos += size;
    if (processedSize)
        *processedSize = size;
    return S_OK;
}

static const Byte kProps[] = {kpidPath, kpidSize, kpidPackSize, kpidMTime, kpidComment};

static const Byte kArcProps[] = {kpidClusterSize,
                                 kpidSectorSize, // actually we need variable to show table size
                                 kpidHeadersSize, kpidUnpackVer, kpidMethod, kpidCharacts};

IMP_IInArchive_Props IMP_IInArchive_ArcProps

    static const CUInt32PCharPair g_IncompatFlags_Characts[] = {{0, "Dirty"},
                                                                {1, "Corrupt"},
                                                                {2, "External_Data_File"},
                                                                {3, "Compression"},
                                                                {4, "Extended_L2"}};

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NCOM::CPropVariant prop;

    switch (propID) {
    case kpidMainSubfile:
        if (!_numSnapshots)
            prop = (UInt32)0;
        break;
    case kpidClusterSize:
        prop = (UInt32)1 << _clusterBits;
        break;
    case kpidSectorSize:
        prop = (UInt32)1 << (_numMidBits + _l2EntryBits);
        break;
    case kpidHeadersSize:
        if (!_numSnapshots)
            prop = _table.Size() + (UInt64)_dir.Size() * 8;
        break;
    case kpidPhySize:
        if (_phySize)
            prop = _phySize;
        break;
    case kpidUnpackVer:
        prop = _version;
        break;
    case kpidCharacts: {
        if (_incompatFlags) {
            AString s("incompatible: ");
            // we need to show also high 32-bits.
            s += FlagsToString(g_IncompatFlags_Characts, Z7_ARRAY_SIZE(g_IncompatFlags_Characts),
                               (UInt32)_incompatFlags);
            prop = s;
        }
        break;
    }
    case kpidMethod: {
        AString s;

        if (_compressionType) {
            if (_compressionType == 1)
                s += "ZSTD";
            else {
                s += "Compression:";
                s.Add_UInt32(_compressionType);
            }
        } else if (_needCompression)
            s.Add_OptSpaced("Deflate");

        if (_cryptMethod) {
            s.Add_Space_if_NotEmpty();
            if (_cryptMethod == 1)
                s += "AES";
            if (_cryptMethod == 2)
                s += "LUKS";
            else {
                s += "Encryption:";
                s.Add_UInt32(_cryptMethod);
            }
        }
        if (!s.IsEmpty())
            prop = s;
        break;
    }

    case kpidErrorFlags: {
        UInt32 v = 0;
        if (!_isArc)
            v |= kpv_ErrorFlags_IsNotArc;
        if (_unsupported)
            v |= kpv_ErrorFlags_UnsupportedMethod;
        // if (_headerError) v |= kpv_ErrorFlags_HeadersError;
        if (!Stream && v == 0)
            v = kpv_ErrorFlags_HeadersError;
        if (v)
            prop = v;
        break;
    }
    case kpidError:
        if (!_errorMessage.IsEmpty())
            prop = _errorMessage;
        break;
    }

    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (index > _numSnapshots)
        return E_INVALIDARG;
    NCOM::CPropVariant prop;

    switch (propID) {
    case kpidSize:
        prop = index ? _snapshots[index - 1].disk_size : _size;
        break;
    case kpidPackSize:
        // Snapshot images share physical clusters; do not count the source
        // file's bytes once per snapshot when clients sum packed sizes.
        if (!index)
            prop = _phySize;
        break;
    case kpidExtension:
        prop = (index == 0 && _imgExt ? _imgExt : "img");
        break;
    case kpidPath:
        if (_numSnapshots) {
            AString path;
            if (index) {
                path = "snapshot-";
                path.Add_UInt32(index);
                path += ".img";
            } else
                path = "current.img";
            prop = path;
        }
        break;
    case kpidMTime:
        if (index) {
            const QCowSnapshot &sn = _snapshots[index - 1];
            const UInt32 ns = sn.date_nsec < 1000000000 ? sn.date_nsec : 0;
            const UInt64 time = NTime::UnixTime64_To_FileTime64(sn.date_sec) + ns / 100;
            FILETIME ft;
            ft.dwLowDateTime = (DWORD)time;
            ft.dwHighDateTime = (DWORD)(time >> 32);
            prop.SetAsTimeFrom_FT_Prec_Ns100(ft, k_PropVar_TimePrec_1ns, ns % 100);
        }
        break;
    case kpidComment:
        if (index) {
            UString name;
            if (ConvertUTF8ToUnicode(AString(_snapshots[index - 1].name), name))
                prop = name;
        }
        break;
    }

    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *openCallback) {
    UInt64 buf64[0x70 / 8];
    RINOK(ReadStream_FALSE(stream, buf64, sizeof(buf64)))
    const void *buf = (const void *)buf64;
    // signature: { 'Q', 'F', 'I', 0xFB }
    if (*(const UInt32 *)buf != Z7_CONV_BE_TO_NATIVE_CONST32(0x514649fb))
        return S_FALSE;
    _version = Get32((const Byte *)(const void *)buf64 + 4);
    if (_version < 1 || _version > 3)
        return S_FALSE;

    const UInt64 k_UncompressedSize_MAX = (UInt64)1 << 60;
    const UInt64 k_CompressedSize_MAX = (UInt64)1 << 60;

    _size = Get64((const Byte *)(const void *)buf64 + 0x18);
    if (_useSnapshot)
        _size = _snapshotDiskSize;
    if (_size > k_UncompressedSize_MAX)
        return S_FALSE;
    size_t l1Size;
    UInt32 headerSize;
    _l2EntryBits = 3;

    if (_version == 1) {
        // _mTime = Get32((const Byte *)(const void *)buf64 + 0x14); // is unused in most images
        _clusterBits = ((const Byte *)(const void *)buf64)[0x20];
        _numMidBits = ((const Byte *)(const void *)buf64)[0x21];
        if (_clusterBits < 9 || _clusterBits > 30)
            return S_FALSE;
        if (_numMidBits < 1 || _numMidBits > 28)
            return S_FALSE;
        _cryptMethod = Get32((const Byte *)(const void *)buf64 + 0x24);
        const unsigned numBits2 = _clusterBits + _numMidBits;
        const UInt64 l1Size64 = (_size + (((UInt64)1 << numBits2) - 1)) >> numBits2;
        if (l1Size64 > ((UInt32)1 << 31))
            return S_FALSE;
        l1Size = (size_t)l1Size64;
        headerSize = 0x30;
    } else {
        _clusterBits = Get32((const Byte *)(const void *)buf64 + 0x14);
        if (_clusterBits < 9 || _clusterBits > 30)
            return S_FALSE;
        _numMidBits = _clusterBits - 3;
        _cryptMethod = Get32((const Byte *)(const void *)buf64 + 0x20);
        l1Size = Get32((const Byte *)(const void *)buf64 + 0x24);
        if (_useSnapshot)
            l1Size = _snapshotL1Size;
        headerSize = 0x48;
        if (_version >= 3) {
            _incompatFlags = Get64((const Byte *)(const void *)buf64 + 0x48);
            // const UInt64 CompatFlags    = Get64((const Byte *)(const void *)buf64 + 0x50);
            // const UInt64 AutoClearFlags = Get64((const Byte *)(const void *)buf64 + 0x58);
            // const UInt32 RefCountOrder = Get32((const Byte *)(const void *)buf64 + 0x60);
            headerSize = 0x68;
            const UInt32 headerSize2 = Get32((const Byte *)(const void *)buf64 + 0x64);
            if (headerSize2 < 0x68 || headerSize2 > ((UInt32)1 << _clusterBits) ||
                (headerSize2 & 7))
                return S_FALSE;
            if (headerSize < headerSize2)
                headerSize = headerSize2;
            if (headerSize2 >= 0x68 + 1)
                _compressionType = ((const Byte *)(const void *)buf64)[0x68];
            if ((_compressionType != 0) != ((_incompatFlags & 8) != 0))
                return S_FALSE;
            _extendedL2 = (_incompatFlags & 16) != 0;
            if (_extendedL2) {
                if (_clusterBits < 14)
                    return S_FALSE;
                _l2EntryBits = 4;
                _numMidBits = _clusterBits - _l2EntryBits;
            }
        }
        const unsigned l1Bits = _clusterBits + _numMidBits;
        if ((UInt64)l1Size < ((_size + (((UInt64)1 << l1Bits) - 1)) >> l1Bits))
            return S_FALSE;

        const UInt64 refOffset =
            Get64((const Byte *)(const void *)buf64 + 0x30); // must be aligned for cluster
        const UInt32 refClusters = Get32((const Byte *)(const void *)buf64 + 0x38);
        if (refClusters) {
            if (refOffset > k_CompressedSize_MAX)
                return S_FALSE;
            const UInt64 numBytes = (UInt64)refClusters << _clusterBits;
            const UInt64 end = refOffset + numBytes;
            if (end > k_CompressedSize_MAX)
                return S_FALSE;
            /*
            CByteBuffer refs;
            refs.Alloc(numBytes);
            RINOK(InStream_SeekSet(stream, refOffset))
            RINOK(ReadStream_FALSE(stream, refs, numBytes));
            */
            if (_phySize < end)
                _phySize = end;
            /*
            for (size_t i = 0; i < numBytes; i += 2)
            {
              UInt32 v = GetBe16((const Byte *)refs + (size_t)i);
              if (v == 0)
                continue;
            }
            */
        }
    }

    const UInt64 l1Offset =
        _useSnapshot ? _snapshotL1Offset : Get64((const Byte *)(const void *)buf64 + 0x28);
    if ((l1Size && l1Offset < headerSize) || l1Offset > k_CompressedSize_MAX ||
        (_version >= 2 && (l1Offset & (((UInt64)1 << _clusterBits) - 1))))
        return S_FALSE;
    if (_phySize < headerSize)
        _phySize = headerSize;

    // we use 32 MiB limit for L1 size, as QEMU with QCOW_MAX_L1_SIZE limit.
    if (l1Size > (1u << 22)) // if (l1Size > (1u << (sizeof(size_t) * 8 - 4)))
        return S_FALSE;

    _isArc = true;
    {
        // Dirty/corrupt flags are permitted for read-only access.
        if (_incompatFlags & ~(UInt64)0x1F) {
            _unsupported = true;
            return S_FALSE;
        }
    }

    UInt64 fileSize = 0;
    RINOK(InStream_GetSize_SeekToBegin(stream, fileSize))

    const size_t clusterSize = (size_t)1 << _clusterBits;
    const UInt64 backOffset = Get64((const Byte *)(const void *)buf64 + 8);
    const UInt32 backSize = Get32((const Byte *)(const void *)buf64 + 0x10);
    if (backOffset) {
        if (backSize == 0 || backSize > 1023 || backOffset > fileSize ||
            backSize > fileSize - backOffset ||
            (_version >= 2 && (backOffset < headerSize || backOffset > clusterSize ||
                               backSize > clusterSize - backOffset)))
            return S_FALSE;
        CByteBuffer name;
        name.Alloc((size_t)backSize + 1);
        RINOK(InStream_SeekSet(stream, backOffset))
        RINOK(ReadStream_FALSE(stream, name, backSize))
        if (memchr(name, 0, backSize))
            return S_FALSE;
        name[backSize] = 0;
        if (!ConvertUTF8ToUnicode(AString((const char *)(const Byte *)name), _backingName))
            return S_FALSE;
    }
    if (_version >= 2) {
        const UInt64 extEnd = backOffset ? backOffset : clusterSize;
        const auto read = [](void *opaque, uint64_t offset, void *data, size_t size) -> int32_t {
            auto *input = static_cast<IInStream *>(opaque);
            RINOK(InStream_SeekSet(input, offset))
            return ReadStream_FALSE(input, data, size);
        };
        RINOK(qcow2_read_extensions(read, stream, headerSize, extEnd, fileSize, &_extensions))
        if (_extensions.has_crypto && _cryptMethod != 2)
            return S_FALSE;
        if (!_isDiskStream) {
            const UInt32 numSnapshots = Get32((const Byte *)(const void *)buf64 + 0x3C);
            struct CReadContext {
                IInStream *Input;
                IArchiveOpenCallback *Callback;
            } context = {stream, openCallback};
            const auto readSnapshot = [](void *opaque, uint64_t offset, void *data,
                                         size_t size) -> int32_t {
                auto &ctx = *static_cast<CReadContext *>(opaque);
                if (ctx.Callback) {
                    RINOK(ctx.Callback->SetCompleted(NULL, NULL))
                }
                RINOK(InStream_SeekSet(ctx.Input, offset))
                return ReadStream_FALSE(ctx.Input, data, size);
            };
            uint64_t end;
            RINOK(qcow2_read_snapshots(
                readSnapshot, &context, Get64((const Byte *)(const void *)buf64 + 0x40),
                numSnapshots, _size, fileSize, _clusterBits, &_snapshots, &end))
            _numSnapshots = numSnapshots;
            if (_phySize < end)
                _phySize = end;
            if (_numSnapshots)
                _phySize = fileSize;
        }
    }
    if (_sourceName.IsEmpty() && openCallback) {
        CMyComPtr<IArchiveOpenVolumeCallback> volumes;
        openCallback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&volumes);
        if (volumes) {
            NCOM::CPropVariant name;
            RINOK(volumes->GetProperty(kpidName, &name))
            if (name.vt == VT_BSTR)
                _sourceName = name.bstrVal;
        }
    }
    _externalData = (_incompatFlags & 4) != 0;
    if (_version >= 3 && (Get64((const Byte *)(const void *)buf64 + 0x58) & 2) &&
        (!_externalData || backOffset))
        return S_FALSE;
    _dataSize = fileSize;
    if (_externalData && !_isDiskStream) {
        if (!_extensions.image_data_file || !_extensions.image_data_file[0]) {
            _errorMessage = L"Missing external data file name";
            return S_FALSE;
        }
        UString name, resolved;
        if (!ConvertUTF8ToUnicode(AString(_extensions.image_data_file), name))
            return S_FALSE;
        RINOK(OpenReference(openCallback, name, _dataStream, resolved))
        RINOK(InStream_GetSize_SeekToBegin(_dataStream, _dataSize))
    }
    if (_externalData && _isDiskStream) {
        if (!_dataStream)
            return S_FALSE;
        RINOK(InStream_GetSize_SeekToBegin(_dataStream, _dataSize))
    }
    const size_t t1SizeBytes = (size_t)l1Size << 3;
    {
        const UInt64 end = l1Offset + t1SizeBytes;
        if (end > k_CompressedSize_MAX)
            return S_FALSE;
        // we need to use align end for empty qcow files
        // some files has no cluster alignment padding at the end
        // but has sector alignment
        // end = (end + clusterSize - 1) >> _clusterBits << _clusterBits;
        if (_phySize < end)
            _phySize = end;
        if (end > fileSize)
            return S_FALSE;
        if (_phySize < fileSize) {
            const UInt64 end2 = (end + 511) & ~(UInt64)511;
            if (end2 == fileSize)
                _phySize = end2;
        }
    }
    CObjArray<UInt64> table64(l1Size);
    {RINOK(InStream_SeekSet(stream, l1Offset))
         RINOK(ReadStream_FALSE(stream, table64, t1SizeBytes))}

    _compressedFlag = (_version <= 1) ? ((UInt64)1 << 63) : ((UInt64)1 << 62);
    const UInt64 offsetMask = _compressedFlag - 1;
    const size_t midSize = (size_t)1 << (_numMidBits + _l2EntryBits);
    size_t numTables = 0;
    size_t i;

    for (i = 0; i < l1Size; i++) {
        const UInt64 entry = Get64(table64 + (size_t)i);
        if (_version >= 2 && (entry & ~(L2E_OFFSET_MASK | QCOW_OFLAG_COPIED)))
            return S_FALSE;
        const UInt64 v = Get64(table64 + (size_t)i) & offsetMask;
        if (!v)
            continue;
        if (_version >= 2 && (v & (clusterSize - 1)))
            return S_FALSE;
        numTables++;
        const UInt64 end = v + midSize;
        if (end > k_CompressedSize_MAX)
            return S_FALSE;
        if (_phySize < end)
            _phySize = end;
        if (end > fileSize)
            return S_FALSE;
    }

    if (numTables) {
        const size_t size = (size_t)numTables << (_numMidBits + _l2EntryBits);
        if (size >> (_numMidBits + _l2EntryBits) != numTables)
            return E_OUTOFMEMORY;
        _table.Alloc(size);
        if (!_table.IsAllocated())
            return E_OUTOFMEMORY;
        if (openCallback) {
            const UInt64 totalBytes = size;
            RINOK(openCallback->SetTotal(NULL, &totalBytes))
        }
    }

    _dir.SetSize((unsigned)l1Size);

    UInt32 curTable = 0;

    for (i = 0; i < l1Size; i++) {
        Byte *buf2;
        {
            const UInt64 v = Get64(table64 + (size_t)i) & offsetMask;
            if (v == 0) {
                _dir[i] = kEmptyDirItem;
                continue;
            }
            _dir[i] = curTable;
            const size_t tableOffset = (size_t)curTable << (_numMidBits + _l2EntryBits);
            buf2 = (Byte *)_table + tableOffset;
            curTable++;
            if (openCallback && (tableOffset & 0xFFFFF) == 0) {
                const UInt64 numBytes = tableOffset;
                RINOK(openCallback->SetCompleted(NULL, &numBytes))
            }
            RINOK(InStream_SeekSet(stream, v))
            RINOK(ReadStream_FALSE(stream, buf2, midSize))
        }

        for (size_t k = 0; k < midSize; k += (size_t)1 << _l2EntryBits) {
            const UInt64 v = Get64((const Byte *)buf2 + (size_t)k);
            const UInt64 bitmap = _extendedL2 ? Get64(buf2 + k + 8) : 0;
            if (_version >= 2) {
                if (v & _compressedFlag) {
                    if (_externalData || bitmap || (v & QCOW_OFLAG_COPIED))
                        return S_FALSE;
                } else {
                    const UInt64 allowed = L2E_OFFSET_MASK | QCOW_OFLAG_COPIED |
                                           ((_version >= 3 && !_extendedL2) ? QCOW_OFLAG_ZERO : 0);
                    if ((v & ~allowed) || ((v & L2E_OFFSET_MASK) & (clusterSize - 1)) ||
                        qcow2_get_subcluster_type(_extendedL2, _externalData, v, bitmap, 0) ==
                            QCOW2_SUBCLUSTER_INVALID)
                        return S_FALSE;
                }
            }
            if (v == 0)
                continue;
            UInt64 offset = v & offsetMask;
            size_t dataSize = clusterSize;

            if (v & _compressedFlag) {
                if (_version <= 1) {
                    offset = qcow_get_compressed_cluster(v, _clusterBits, &dataSize);
                    if (dataSize == 0)
                        return S_FALSE;
                } else {
                    const unsigned numOffsetBits = 62 - (_clusterBits - 8);
                    dataSize = ((size_t)(offset >> numOffsetBits) + 1) << 9;
                    offset &= ((UInt64)1 << numOffsetBits) - (1 << 9);
                    if (offset >= ((UInt64)1 << 56))
                        return S_FALSE;
                }
                _needCompression = true;
            } else {
                if (_extendedL2) {
                    // QEMU can leave the trailing unallocated subclusters
                    // beyond EOF. Bound only the last allocated subcluster.
                    dataSize = 0;
                    for (UInt32 allocated = (UInt32)bitmap; allocated; allocated >>= 1)
                        dataSize += clusterSize >> 5;
                }
                const UInt32 low = (UInt32)v & 511;
                if (low) {
                    // version_3 supports zero clusters
                    if (_version < 3 || low != 1) {
                        _unsupported = true;
                        return S_FALSE;
                    }
                }
            }

            const UInt64 end = offset + dataSize;
            if (_version == 1 && end > fileSize)
                return S_FALSE;
            if (_version >= 2) {
                const QCow2SubclusterType type =
                    qcow2_get_subcluster_type(_extendedL2, _externalData, v, bitmap, 0);
                // Only allocated data is dereferenced. Zero/preallocated ranges
                // need no host bytes; extended allocation can start after bit 0.
                const bool readsData =
                    (v & _compressedFlag) ||
                    (_extendedL2 ? (UInt32)bitmap != 0 : type == QCOW2_SUBCLUSTER_NORMAL);
                if (readsData && end > _dataSize)
                    return S_FALSE;
                if (_externalData &&
                    (type == QCOW2_SUBCLUSTER_NORMAL || type == QCOW2_SUBCLUSTER_ZERO_ALLOC ||
                     type == QCOW2_SUBCLUSTER_UNALLOCATED_ALLOC)) {
                    const unsigned l1Bits = _numMidBits + _clusterBits;
                    if ((UInt64)i > ((UInt64)(Int64)-1 >> l1Bits))
                        return S_FALSE;
                    const UInt64 guest =
                        ((UInt64)i << l1Bits) + ((UInt64)(k >> _l2EntryBits) << _clusterBits);
                    if ((v & L2E_OFFSET_MASK) != guest)
                        return S_FALSE;
                }
            }
            if (!_externalData && _phySize < end)
                _phySize = end;
        }
    }

    if (curTable != numTables)
        return E_FAIL;

    if (_cryptMethod)
        _unsupported = true;
    if (_compressionType > 1)
        _unsupported = true;

    _sourceStream = stream;
    // The original extent stream restores source position on each read. Each
    // image mapping owns a cursor even when source bytes are shared.
    CMyComPtr2_Create<IInStream, CExtentsStream> cursor;
    cursor->Stream = stream;
    CSeekExtent extent;
    extent.Virt = 0;
    extent.Phy = 0;
    cursor->Extents.Add(extent);
    extent.Virt = fileSize;
    cursor->Extents.Add(extent);
    cursor->Init();
    Stream = cursor;
    if (!_backingName.IsEmpty() && !_isDiskStream) {
        RINOK(OpenParent(openCallback))
    }
    return S_OK;
}

// The volume callback, ownership and depth guard follow VhdxHandler::OpenParent.
// Names remain relative to the selected image; the callback owns path validation.
HRESULT CHandler::OpenReference(IArchiveOpenCallback *callback, const UString &name,
                                CMyComPtr<IInStream> &input, UString &resolved) {
    if (!callback)
        return S_FALSE;
    CMyComPtr<IArchiveOpenVolumeCallback> volumes;
    callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&volumes);
    if (!volumes)
        return S_FALSE;
    resolved = _sourceName.Left(_sourceName.ReverseFind_PathSepar() + 1);
    resolved += name;
    const HRESULT result = volumes->GetStream(resolved, &input);
    if (result != S_OK || !input) {
        _errorMessage = L"Missing or inaccessible image dependency: ";
        _errorMessage += resolved;
        return result == S_OK ? S_FALSE : result;
    }
    return S_OK;
}

HRESULT CHandler::OpenParent(IArchiveOpenCallback *callback) {
    if (_level >= (1 << 8) - 1) {
        _errorMessage = L"Too many parent levels";
        return S_FALSE;
    }
    CMyComPtr<IInStream> input;
    UString name;
    RINOK(OpenReference(callback, _backingName, input, name))
    for (const CHandler *child = this; child; child = child->_child) {
        if (name.IsEqualTo_NoCase(child->_sourceName)) {
            _errorMessage = L"Cyclic parent archive was blocked";
            return S_FALSE;
        }
    }
    UInt64 size;
    RINOK(InStream_GetSize_SeekToBegin(input, size))
    Byte signature[4];
    size_t count = sizeof(signature);
    RINOK(ReadStream(input, signature, &count))
    RINOK(InStream_SeekSet(input, 0))
    const bool isQcow = count == 4 && memcmp(signature, k_Signature, 4) == 0;
    const char *format = _extensions.image_backing_format;
    if (format && format[0] && strcmp(format, "raw") != 0 && strcmp(format, "qcow") != 0 &&
        strcmp(format, "qcow2") != 0) {
        _unsupported = true;
        _errorMessage = L"Unsupported backing file format";
        return S_FALSE;
    }
    if ((format && strcmp(format, "raw") == 0) || ((!format || !format[0]) && !isQcow)) {
        _backingStream = input;
        _backingSize = size;
        return S_OK;
    }
    CHandler *parent = new CHandler;
    CMyComPtr<IInStream> owner = parent;
    parent->Close();
    parent->_level = _level + 1;
    parent->_child = this;
    parent->_sourceName = name;
    const HRESULT result = parent->Open2(input, callback);
    if (result != S_OK) {
        _errorMessage = parent->_errorMessage;
        _unsupported = parent->_unsupported;
        return result;
    }
    CMyComPtr<ISequentialInStream> sequential;
    RINOK(parent->GetStream(0, &sequential))
    if (!sequential)
        return S_FALSE;
    RINOK(sequential.QueryInterface(IID_IInStream, &_backingStream))
    _backingSize = parent->_size;
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Close()) {
    _table.Free();
    _dir.Free();
    // _cache.Free();
    // _cacheCompressed.Free();
    _phySize = 0;

    _cacheCluster = (UInt64)(Int64)-1;
    _comprPos = 0;
    _comprSize = 0;

    _needCompression = false;
    _isArc = false;
    _unsupported = false;

    _compressionType = 0;
    _incompatFlags = 0;
    _extendedL2 = false;
    _externalData = false;
    _backingStream.Release();
    _dataStream.Release();
    _backingSize = 0;
    _dataSize = 0;
    _backingName.Empty();
    _sourceName.Empty();
    _errorMessage.Empty();
    qcow2_free_extensions(&_extensions);
    qcow2_free_snapshots(_snapshots, _numSnapshots);
    _snapshots = NULL;
    _numSnapshots = 0;
    _sourceStream.Release();
    _isDiskStream = false;
    _useSnapshot = false;

    // CHandlerImg:
    Clear_HandlerImg_Vars();
    Stream.Release();
    return S_OK;
}

HRESULT CHandler::CreateImage(UInt32 index, CHandler *&image, CMyComPtr<IInStream> &owner,
                              IArchiveOpenCallback *callback) {
    if (index > _numSnapshots || !Stream)
        return E_INVALIDARG;
    // QEMU snapshot operations reject external data files: their guest bytes
    // are overwritten in place and cannot represent a historical disk state.
    if (index && _externalData)
        return E_NOTIMPL;
    image = new CHandler;
    owner = image;
    image->Close();
    image->_isDiskStream = true;
    image->_backingStream = _backingStream;
    image->_backingSize = _backingSize;
    image->_dataStream = _dataStream;
    if (index) {
        const QCowSnapshot &sn = _snapshots[index - 1];
        // QEMU snapshot_load_tmp selects the saved L1 without changing disk bytes.
        image->_useSnapshot = true;
        image->_snapshotL1Offset = sn.l1_table_offset;
        image->_snapshotL1Size = sn.l1_size;
        image->_snapshotDiskSize = sn.disk_size;
    }
    RINOK(InStream_SeekSet(_sourceStream, 0))
    return image->Open2(_sourceStream, callback);
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = _numSnapshots + 1;
    return S_OK;
}

// Preserve CHandlerImg's extraction/error projection for each selected image;
// only translate its single-item index and progress into the enclosing archive.
class CSnapshotExtractCallback final : public IArchiveExtractCallback,
                                       public IArchiveOpenCallback,
                                       public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IArchiveExtractCallback, IArchiveOpenCallback)
  public:
    IArchiveExtractCallback *Callback;
    UInt32 Index;
    UInt64 Completed;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return Callback->SetCompleted(&Completed);
    }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        if (!value)
            return Callback->SetCompleted(NULL);
        const UInt64 completed = Completed + *value;
        return Callback->SetCompleted(&completed);
    }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32 mode)) override {
        return Callback->GetStream(Index, stream, mode);
    }
    Z7_COM7F_IMF(PrepareOperation(Int32 mode)) override { return Callback->PrepareOperation(mode); }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        return Callback->SetOperationResult(result);
    }
};

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode,
                               IArchiveExtractCallback *callback)) {
    COM_TRY_BEGIN
    // Indexed selection/aggregate progress follows CHandlerCont::Extract;
    // CHandlerImg::Extract remains the authoritative single-image operation.
    const bool all = numItems == (UInt32)(Int32)-1;
    if (all)
        numItems = _numSnapshots + 1;
    if (!numItems)
        return S_OK;
    if (!all && !indices)
        return E_INVALIDARG;
    UInt64 total = 0;
    for (UInt32 i = 0; i < numItems; i++) {
        const UInt32 index = all ? i : indices[i];
        if (index > _numSnapshots)
            return E_INVALIDARG;
        const UInt64 size = index ? _snapshots[index - 1].disk_size : _size;
        if (size > (UInt64)(Int64)-1 - total)
            return E_INVALIDARG;
        total += size;
    }
    RINOK(callback->SetTotal(total))
    total = 0;
    for (UInt32 i = 0; i < numItems; i++) {
        RINOK(callback->SetCompleted(&total))
        const UInt32 index = all ? i : indices[i];
        CMyComPtr2_Create<IArchiveExtractCallback, CSnapshotExtractCallback> mapped;
        mapped->Callback = callback;
        mapped->Index = index;
        mapped->Completed = total;
        CHandler *image;
        CMyComPtr<IInStream> owner;
        const HRESULT result = CreateImage(index, image, owner, mapped.ClsPtr());
        if (result == S_FALSE || result == E_NOTIMPL) {
            CMyComPtr<ISequentialOutStream> output;
            const Int32 mode = testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
            RINOK(callback->GetStream(index, &output, mode))
            if (testMode || output) {
                RINOK(callback->PrepareOperation(mode))
                RINOK(callback->SetOperationResult(
                    result == E_NOTIMPL ? NExtract::NOperationResult::kUnsupportedMethod
                                        : NExtract::NOperationResult::kDataError))
            }
        } else {
            RINOK(result)
            RINOK(image->CHandlerImg::Extract(NULL, (UInt32)(Int32)-1, testMode, mapped))
        }
        total += index ? _snapshots[index - 1].disk_size : _size;
    }
    return callback->SetCompleted(&total);
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream)) {
    COM_TRY_BEGIN
    *stream = NULL;
    if (_unsupported || !Stream)
        return S_FALSE;
    if (_needCompression) {
        if (_compressionType > 1)
            return S_FALSE;
        _bufInStream.Create_if_Empty();
        _bufOutStream.Create_if_Empty();
        if (_compressionType == 1) {
            if (!_zstd) {
                _zstd = ZstdDec_Create(&g_AlignedAlloc, &g_AlignedAlloc);
                if (!_zstd)
                    return E_OUTOFMEMORY;
            }
        } else {
            _deflateDecoder.Create_if_Empty();
            // QEMU QCOW1 accepts Z_BUF_ERROR when the complete cluster was
            // produced, even without a final Deflate marker in the byte range.
            _deflateDecoder->Set_NeedFinishInput(_version >= 2);
        }
        const size_t clusterSize = (size_t)1 << _clusterBits;
        _cache.AllocAtLeast(clusterSize);
        _cacheCompressed.AllocAtLeast(clusterSize * 2);
    }
    if (!_isDiskStream) {
        CHandler *image;
        CMyComPtr<IInStream> owner;
        RINOK(CreateImage(index, image, owner))
        return image->GetStream(0, stream);
    }
    CMyComPtr<ISequentialInStream> streamTemp = this;
    RINOK(InitAndSeek())
    *stream = streamTemp.Detach();
    return S_OK;
    COM_TRY_END
}

REGISTER_ARC_I("QCOW", "qcow qcow2 qcow2c", NULL, 0xCA, k_Signature, 0, 0, NULL)

} // namespace NQcow
} // namespace NArchive
