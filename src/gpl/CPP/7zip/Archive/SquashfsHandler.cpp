// SquashfsHandler.cpp

#include "7zip/Archive/StdAfx.h"

#include "C/Alloc.h"
#include "C/CpuArch.h"
#include "C/Xz.h"
#include "C/ZstdDec.h"

#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/MyLinux.h"
#include "Common/StringConvert.h"
#include "Common/UTFConvert.h"

#include "Windows/PropVariantUtils.h"
#include "Windows/TimeUtils.h"

#include "7zip/Common/CWrappers.h"
#include "7zip/Common/LimitedStreams.h"
#include "7zip/Common/ProgressUtils.h"
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Common/StreamUtils.h"

#include "7zip/Compress/CopyCoder.h"
#include "7zip/Compress/ZlibDecoder.h"
#include "archiveattributes.h"
#include "squashfsdecoder.h"
#include "squashfsmetadata.h"
#include <algorithm>
#include <errno.h>
#include <map>
#include <memory>
#include <set>
// #include "../Compress/LzmaDecoder.h"

namespace NArchive {
namespace NSquashfs {

static const UInt32 kNumFilesMax = 1 << 28;
static const unsigned kNumDirLevelsMax = 1 << 10;

// Layout: Header, Data, inodes, Directories, Fragments, UIDs, GIDs

#define LE_16(offs, dest) dest = GetUi16(p + (offs))
#define LE_32(offs, dest) dest = GetUi32(p + (offs))
#define LE_64(offs, dest) dest = GetUi64(p + (offs))

#define GET_16(offs, dest) dest = GetUi16(p + (offs))
#define GET_32(offs, dest) dest = GetUi32(p + (offs))
#define GET_64(offs, dest) dest = GetUi64(p + (offs))

static const UInt32 kSignature32_LE = 0x73717368;
#define kMethod_ZLIB 1
#define kMethod_LZO 3
#define kMethod_XZ 4
#define kMethod_LZ4 5
#define kMethod_ZSTD 6

static const char *const k_Methods[] = {"0", "ZLIB", "LZMA", "LZO", "XZ", "LZ4", "ZSTD"};

static const unsigned kMetadataBlockSizeLog = 13;
static const UInt32 kMetadataBlockSize = (1 << kMetadataBlockSizeLog);

enum { kType_IPC, kType_DIR, kType_FILE, kType_LNK, kType_BLK, kType_CHR, kType_FIFO, kType_SOCK };

static const UInt32 k_TypeToMode[] = {0,
                                      MY_LIN_S_IFDIR,
                                      MY_LIN_S_IFREG,
                                      MY_LIN_S_IFLNK,
                                      MY_LIN_S_IFBLK,
                                      MY_LIN_S_IFCHR,
                                      MY_LIN_S_IFIFO,
                                      MY_LIN_S_IFSOCK,
                                      MY_LIN_S_IFDIR,
                                      MY_LIN_S_IFREG,
                                      MY_LIN_S_IFLNK,
                                      MY_LIN_S_IFBLK,
                                      MY_LIN_S_IFCHR,
                                      MY_LIN_S_IFIFO,
                                      MY_LIN_S_IFSOCK};

enum {
    kFlag_UNC_INODES,
    kFlag_UNC_DATA,
    kFlag_CHECK,
    kFlag_UNC_FRAGS,
    kFlag_NO_FRAGS,
    kFlag_ALWAYS_FRAG,
    kFlag_DUPLICATE,
    kFlag_EXPORT
};

static const char *const k_Flags[] = {"UNCOMPRESSED_INODES",
                                      "UNCOMPRESSED_DATA",
                                      "CHECK",
                                      "UNCOMPRESSED_FRAGMENTS",
                                      "NO_FRAGMENTS",
                                      "ALWAYS_FRAGMENTS",
                                      "DUPLICATES_REMOVED",
                                      "EXPORTABLE",
                                      "UNCOMPRESSED_XATTRS",
                                      "NO_XATTRS",
                                      "COMPRESSOR_OPTIONS",
                                      "UNCOMPRESSED_IDS"};

static const UInt32 kNotCompressedBit16 = 1 << 15;
static const UInt32 kNotCompressedBit32 = 1 << 24;

#define GET_COMPRESSED_BLOCK_SIZE(size) ((size) & ~kNotCompressedBit32)
#define IS_COMPRESSED_BLOCK(size) (((size) & kNotCompressedBit32) == 0)

static const UInt32 kHeaderSize4 = 0x60;

struct CHeader {
    UInt32 NumInodes;
    UInt32 CTime;
    UInt32 BlockSize;
    UInt32 NumFrags;
    UInt16 Method;
    UInt16 BlockSizeLog;
    UInt16 Flags;
    UInt16 NumIDs;
    UInt16 Major;
    UInt16 Minor;
    UInt64 RootInode;
    UInt64 Size;
    UInt64 UidTable;
    UInt64 GidTable;
    UInt64 XattrIdTable;
    UInt64 InodeTable;
    UInt64 DirTable;
    UInt64 FragTable;
    UInt64 LookupTable;

    void Parse4(const Byte *p) {
        LE_32(0x08, CTime);
        LE_32(0x0C, BlockSize);
        LE_32(0x10, NumFrags);
        LE_16(0x14, Method);
        LE_16(0x16, BlockSizeLog);
        LE_16(0x18, Flags);
        LE_16(0x1A, NumIDs);
        LE_64(0x20, RootInode);
        LE_64(0x28, Size);
        LE_64(0x30, UidTable);
        LE_64(0x38, XattrIdTable);
        LE_64(0x40, InodeTable);
        LE_64(0x48, DirTable);
        LE_64(0x50, FragTable);
        LE_64(0x58, LookupTable);
        GidTable = 0;
    }

    bool Parse(const Byte *p) {
        if (GetUi32(p) != kSignature32_LE)
            return false;
        LE_32(4, NumInodes);
        LE_16(0x1C, Major);
        LE_16(0x1E, Minor);
        if (Major != 4 || Minor != 0)
            return false;
        Parse4(p);
        return InodeTable < DirTable && DirTable <= Size && BlockSizeLog >= 12 &&
               BlockSizeLog <= 20 && BlockSize == ((UInt32)1 << BlockSizeLog);
    }
    unsigned GetFileNameOffset() const { return 8; }
    unsigned GetSymLinkOffset() const { return 24; }
};

static const UInt32 kFrag_Empty = (UInt32)(Int32)-1;
// static const UInt32 kXattr_Empty = (UInt32)(Int32)-1;

struct CInodeReader {
    void *Context;
    HRESULT (*Read)(void *, UInt64 *, unsigned *, void *, unsigned);
    UInt64 Block;
    unsigned Offset;
    HRESULT Result = S_OK;
    UInt32 Size = 0;
    CByteBuffer Data;

    bool Ensure(UInt32 required) {
        if (required <= Size)
            return true;
        if (Result != S_OK)
            return false;
        if (required > Data.Size()) {
            const UInt64 growth = (UInt64)Data.Size() + Data.Size() / 2 + 64;
            const UInt32 capacity =
                (UInt32)MyMax((UInt64)required, MyMin(growth, (UInt64)UINT32_MAX));
            Data.ChangeSize_KeepData(capacity, Size);
        }
        Result = Read(Context, &Block, &Offset, Data + Size, required - Size);
        if (Result != S_OK)
            return false;
        Size = required;
        return true;
    }
    HRESULT Skip(UInt64 size) {
        while (size) {
            const unsigned length = (unsigned)MyMin(size, (UInt64)65536);
            Result = Read(Context, &Block, &Offset, NULL, length);
            RINOK(Result)
            size -= length;
        }
        return S_OK;
    }
};

struct CNode {
    UInt16 Type;
    UInt16 Mode;
    UInt16 Uid;
    UInt16 Gid;
    UInt32 Frag;
    UInt32 Offset;
    UInt32 MTime;
    UInt32 Number;
    UInt32 NumLinks;
    UInt32 RDev;
    UInt32 Xattr;
    // UInt32 Parent;

    UInt64 FileSize;
    UInt64 StartBlock;
    // UInt64 Sparse;

    UInt32 Parse4(const Byte *p, UInt32 size, const CHeader &_h, UInt32 *needed = NULL,
                  bool headerOnly = false, CInodeReader *reader = NULL);

    bool IsDir() const { return (Type == kType_DIR || Type == kType_DIR + 7); }
    bool IsLink() const { return (Type == kType_LNK || Type == kType_LNK + 7); }
    UInt64 GetSize() const { return IsDir() ? 0 : FileSize; }

    bool ThereAreFrags() const { return Frag != kFrag_Empty; }
    UInt64 GetNumBlocks(const CHeader &_h) const {
        return (FileSize >> _h.BlockSizeLog) +
               (!ThereAreFrags() && (FileSize & (_h.BlockSize - 1)) != 0);
    }
};

UInt32 CNode::Parse4(const Byte *p, UInt32 size, const CHeader &_h, UInt32 *needed, bool headerOnly,
                     CInodeReader *reader) {
    if (needed)
        *needed = 0;
    const auto available = [&](UInt64 required) {
        if (reader && required <= UINT32_MAX) {
            if (!reader->Ensure((UInt32)required))
                return false;
            p = reader->Data;
            size = reader->Size;
        }
        if (required <= size)
            return true;
        if (needed && required <= UINT32_MAX)
            *needed = (UInt32)required;
        return false;
    };
    if (!available(20))
        return 0;
    LE_16(0, Type);
    LE_16(2, Mode);
    LE_16(4, Uid);
    LE_16(6, Gid);
    LE_32(8, MTime);
    LE_32(12, Number);
    if ((Mode & MY_LIN_S_IFMT) != 0 || GetUi32(p + 12) == 0)
        return 0;

    Xattr = (UInt32)-1;
    NumLinks = 1;
    FileSize = 0;
    StartBlock = 0;

    if (Type == kType_FILE || Type == kType_FILE + 7) {
        UInt32 offset;
        if (Type == kType_FILE) {
            if (!available(32))
                return 0;
            LE_32(16, StartBlock);
            LE_32(20, Frag);
            LE_32(24, Offset);
            LE_32(28, FileSize);
            offset = 32;
        } else {
            if (!available(56))
                return 0;
            LE_64(16, StartBlock);
            LE_64(24, FileSize);
            // LE_64 (32, Sparse);
            LE_32(40, NumLinks);
            LE_32(44, Frag);
            LE_32(48, Offset);
            LE_32(52, Xattr);
            offset = 56;
        }
        if ((Int64)FileSize < 0 ||
            (ThereAreFrags() && ((FileSize & (_h.BlockSize - 1)) == 0 || Offset > _h.BlockSize ||
                                 (FileSize & (_h.BlockSize - 1)) > _h.BlockSize - Offset)))
            return 0;
        UInt64 pos = GetNumBlocks(_h) * 4 + offset;
        if (headerOnly)
            return offset;
        return available(pos) ? (UInt32)pos : 0;
    }

    if (Type == kType_DIR) {
        if (!available(32))
            return 0;
        LE_32(16, StartBlock);
        LE_32(20, NumLinks);
        LE_16(24, FileSize);
        LE_16(26, Offset);
        // LE_32 (28, Parent);
        return 32;
    }

    LE_32(16, NumLinks);

    if (Type == kType_DIR + 7) {
        if (!available(40))
            return 0;
        LE_32(20, FileSize);
        LE_32(24, StartBlock);
        // LE_32 (28, Parent);
        UInt32 iCount;
        LE_16(32, iCount);
        LE_16(34, Offset);
        LE_32(36, Xattr);

        if (headerOnly)
            return 40;
        UInt32 pos = 40;
        for (UInt32 i = 0; i < iCount; i++) {
            // UInt32 index
            // UInt32 startBlock
            if (!available(pos + 12))
                return 0;
            UInt32 nameLen = GetUi32(p + pos + 8);
            if (nameLen >= 256)
                return 0;
            pos += 12 + nameLen + 1;
            if (!available(pos))
                return 0;
        }
        return pos;
    }

    unsigned offset = 20;
    switch (Type) {
    case kType_FIFO:
    case kType_FIFO + 7:
    case kType_SOCK:
    case kType_SOCK + 7:
        break;
    case kType_LNK:
    case kType_LNK + 7: {
        if (!available(24))
            return 0;
        UInt32 len;
        LE_32(20, len);
        FileSize = len;
        if (len > (1 << 16))
            return 0;
        offset = len + 24;
        if (!available(offset))
            return 0;
        break;
    }
    case kType_BLK:
    case kType_BLK + 7:
    case kType_CHR:
    case kType_CHR + 7:
        if (!available(24))
            return 0;
        LE_32(20, RDev);
        offset = 24;
        break;
    default:
        return 0;
    }

    if (Type >= 8) {
        if (!available(offset + 4))
            return 0;
        LE_32(offset, Xattr);
        offset += 4;
    }
    return offset;
}

struct CItem {
    int Node;
    int Parent;
    int Attribute;
    UInt32 AttributeSuffix;
    UInt16 UnavailableType;
    bool RecoveredInode;

    void Construct() {
        Node = -1;
        Parent = -1;
        Attribute = -1;
        AttributeSuffix = 0;
        UnavailableType = 0;
        RecoveredInode = false;
    }
};

struct CFrag {
    UInt64 StartBlock;
    UInt32 Size;
    bool Available = true;
};

// clang-format off
struct CTempItem;
Z7_CLASS_IMP_CHandler_IInArchive_2(
  IInArchiveGetStream,
  IArchiveGetRawProps
)

  CRecordVector<CItem> _items;
  CObjectVector<AString> _itemNames;
  CRecordVector<CNode> _nodes;
  CRecordVector<bool> _activeDirs;
  CRecordVector<CFrag> _frags;
  CByteBuffer _uids;
  CHeader _h;

  UInt64 _sizeCalculated;
  CMyComPtr<IInStream> _stream;

  IArchiveOpenCallback *_openCallback;
  HRESULT _directoryCallbackResult = S_OK;

  UInt32 _openCodePage;
  int _nodeIndex;

  CByteBuffer _cachedBlock;
  UInt64 _cachedBlockStartPos;
  UInt64 _processedPackSize = 0;
  UInt32 _cachedPackBlockSize;
  UInt32 _cachedUnpackBlockSize;
  bool _cachedBlockCompressed;

  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> _limitedInStream;
  CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> _outStream;
  CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> _dynOutStream;

  // CMyComPtr2<ICompressCoder, NCompress::NLzma::CDecoder> _lzmaDecoder;
  CMyComPtr2<ICompressCoder, NCompress::NZlib::CDecoder> _zlibDecoder;

  CXzUnpacker _xz;
  CZstdDecHandle _zstd;

  CByteBuffer _inputBuffer;
  sevenzip_squashfs_decoder *_linuxDecoder = NULL;
  sevenzip_squashfs_metadata *_linuxMetadata = NULL;
  sevenzip_squashfs_file *_linuxFile = NULL;
  CRecordVector<UInt64> _nodeReferences;
  std::vector<std::vector<Byte>> _nodeLinks;
  CByteBuffer _inodeSnapshot;
  UInt32 _inodeReads = 0;
  std::set<unsigned> _incompleteBlocklists;
  HRESULT CreateLinuxFile(unsigned node, sevenzip_squashfs_file **file);
  HRESULT ReadLinuxFileBlock(sevenzip_squashfs_file *file, UInt64 index,
                             uint64_t *start, unsigned *size);
  HRESULT GetLinuxPackSize(unsigned node, UInt64 &total);
  HRESULT _linuxReadResult = S_OK;
  UInt64 _metadataReadEnd = 0;
  static int ReadLinux(void *opaque, uint64_t offset, void *data, size_t size);
  HRESULT InitLinuxDecoder();
  static int ReadLinuxMetadata(void *, uint64_t, void *, unsigned *, uint64_t *);
  HRESULT LoadLinuxTables(UInt64 &directoryEnd);
  HRESULT LoadFragment(CNode &node);
  HRESULT LoadId(UInt32 index);
  HRESULT LoadNode(unsigned index);
  CRecordVector<bool> _loadedNodes;
  std::set<UInt32> _loadedIds;
  std::map<UInt32, UInt32> _fragmentIndexes;
  HRESULT LinuxResult(int result) const;
  using CAttributes = std::shared_ptr<std::vector<ArchiveAttribute>>;
  std::vector<CAttributes> _attributes;
  std::map<UInt32, CAttributes> _attributeSets;
  std::set<UInt32> _incompleteAttributes;
  std::set<UInt32> _unavailableIds;
  std::set<UInt32> _incompleteDirectories;
  std::map<UInt32, std::pair<UInt64, UInt16>> _inodeReferences;
  std::map<UInt32, std::pair<UInt64, UInt16>> _inventoryReferences;
  using CInodePosition = std::pair<UInt64, unsigned>;
  std::map<UInt64, std::pair<unsigned, CInodePosition>> _inodeNodes;
  std::set<UInt64> _badInodeReferences;
  bool _inodeTableIncomplete = false;
  bool _inodeInventoryScanned = false;
  HRESULT ScanCachedInodes(bool forRecovery = false);
  HRESULT EnsureInodeInventory();
  UInt64 _directoryDataEnd = 0;
  static HRESULT ReadInodeBytes(void *, UInt64 *, unsigned *, void *, unsigned);
  HRESULT ReadInode(UInt64 reference, CNode &node, std::vector<Byte> &link,
                    CInodePosition &end, bool complete);
  HRESULT AddInode(UInt64 reference, const CNode &node, const std::vector<Byte> &link,
                   const CInodePosition &end, int &index);
  HRESULT ReadNode(UInt64 reference, int &nodeIndex);
  bool _inodeMappingLimitations = false;
  bool InodeIdentityUnavailable(const CItem &item) const {
    if (item.RecoveredInode)
      return true;
    if (item.Node < 0)
      return false;
    const auto found = _inodeReferences.find(_nodes[item.Node].Number);
    return found != _inodeReferences.end() && found->second.first == UINT64_MAX;
  }
  bool _ownershipLimitations = false;
  bool _fragmentLimitations = false;
  bool _indexLimitations = false;
  bool OwnershipUnavailable(const CNode &node) const {
    return (node.Uid >= _h.NumIDs || node.Gid >= _h.NumIDs ||
        _unavailableIds.count(node.Uid) || _unavailableIds.count(node.Gid));
  }
  bool FragmentUnavailable(const CNode &node) const {
    return (node.Type == kType_FILE || node.Type == kType_FILE + 7) && node.ThereAreFrags() &&
        (node.Frag >= _frags.Size() || !_frags[node.Frag].Available);
  }
  AString _rootAttributeName;
  struct CAttributeContext { CHandler *Owner; unsigned Node; HRESULT Result; };
  static int EmitAttribute(void *, const char *, const void *, size_t) noexcept;
  HRESULT AddAttributeItems(int root);
  UInt64 GetItemSize(unsigned index) const;
  HRESULT FindNode(UInt32 block, UInt32 offset, int &nodeIndex);
  HRESULT ReadDirItems(int parent, const CNode &node, CRecordVector<CTempItem> &items,
                       HRESULT &callbackResult);
  bool ItemIsDir(unsigned index) const;

  void ClearCache()
  {
    _cachedBlockStartPos = 0;
    _cachedPackBlockSize = 0;
    _cachedUnpackBlockSize = 0;
    _cachedBlockCompressed = false;
  }

  HRESULT Seek2(UInt64 offset)
  {
    if (offset > _h.Size)
      return S_FALSE;
    return InStream_SeekSet(_stream, offset);
  }

  HRESULT Decompress(ISequentialOutStream *outStream, Byte *outBuf, bool *outBufWasWritten, UInt32 *outBufWasWrittenSize,
      UInt32 inSize, UInt32 outSizeMax);
  HRESULT ReadMetadataBlock(UInt32 &packSize, bool *boundaryKnown = NULL,
                            CDynBufSeqOutStream *output = NULL);
  HRESULT ReadDirectoryData(UInt64 end);

  HRESULT OpenDir(int parent, UInt32 startBlock, UInt32 offset, unsigned level, int &nodeIndex);
  HRESULT Open2(IInStream *inStream);
  AString GetPath(unsigned index) const;
  const Byte *GetItemName(unsigned index, unsigned &size) const;
  bool GetPackSize(unsigned index, UInt64 &res);
  HRESULT CreateReadStream(UInt32 index, ISequentialInStream **stream, CHandler **reader = NULL);

public:
  CHandler();
  ~CHandler()
  {
    sevenzip_squashfs_decoder_destroy(_linuxDecoder);
    sevenzip_squashfs_metadata_destroy(_linuxMetadata);
    sevenzip_squashfs_file_destroy(_linuxFile);
    XzUnpacker_Free(&_xz);
    if (_zstd)
      ZstdDec_Destroy(_zstd);
  }

  HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize);
};
// clang-format on

CHandler::CHandler() : _zstd(NULL) { XzUnpacker_Construct(&_xz, &g_Alloc); }

int CHandler::ReadLinux(void *opaque, uint64_t offset, void *data, size_t size) {
    CHandler *handler = static_cast<CHandler *>(opaque);
    if (offset > handler->_h.Size || size > handler->_h.Size - offset) {
        handler->_linuxReadResult = S_FALSE;
        return -1;
    }
    try {
        HRESULT result = handler->Seek2(offset);
        if (result == S_OK)
            result = ReadStream_FALSE(handler->_stream, data, size);
        handler->_linuxReadResult = result;
        return result == S_OK            ? 0
               : result == E_ABORT       ? -ECANCELED
               : result == E_OUTOFMEMORY ? -ENOMEM
               : result == E_NOTIMPL     ? -95
                                         : -EIO;
    } catch (...) {
        // Match the upstream COM boundary without unwinding through C-owned
        // option buffers and decoder setup allocations.
        handler->_linuxReadResult = E_OUTOFMEMORY;
        return -ENOMEM;
    }
}

HRESULT CHandler::InitLinuxDecoder() {

    _linuxReadResult = S_OK;
    const int result = sevenzip_squashfs_decoder_create(_h.Method, _h.BlockSize, _h.Flags, _h.Size,
                                                        ReadLinux, this, &_linuxDecoder);
    if (_linuxReadResult != S_OK)
        return _linuxReadResult;
    return result == 0 ? S_OK : result == -12 ? E_OUTOFMEMORY : result == -95 ? E_NOTIMPL : S_FALSE;
}

HRESULT CHandler::LinuxResult(int result) const {
    if (result == -ENOMEM)
        return E_OUTOFMEMORY;
    if (result == -ECANCELED)
        return E_ABORT;
    if (result == -95)
        return E_NOTIMPL;
    if (_linuxReadResult != S_OK)
        return _linuxReadResult;
    return result >= 0 ? S_OK : S_FALSE;
}

int CHandler::ReadLinuxMetadata(void *opaque, uint64_t block, void *data, unsigned *size,
                                uint64_t *next) {
    CHandler *handler = static_cast<CHandler *>(opaque);
    const UInt64 end = handler->_metadataReadEnd
                           ? MyMin(handler->_metadataReadEnd, handler->_h.Size)
                           : handler->_h.Size;
    if (block > end) {
        handler->_linuxReadResult = S_FALSE;
        return -5;
    }
    try {
        HRESULT result = handler->Seek2(block);
        UInt32 packed = (UInt32)MyMin(end - block, (UInt64)kMetadataBlockSize + 3);
        handler->_dynOutStream->Init();
        if (result == S_OK)
            result = handler->ReadMetadataBlock(packed);
        if (result == S_OK) {
            const size_t actual = handler->_dynOutStream->GetSize();
            if (actual > *size)
                result = S_FALSE;
            else {
                memcpy(data, handler->_dynOutStream->GetBuffer(), actual);
                *size = (unsigned)actual;
                *next = block + packed;
            }
        }
        handler->_linuxReadResult = result;
        return result == S_OK            ? 0
               : result == E_ABORT       ? -ECANCELED
               : result == E_OUTOFMEMORY ? -ENOMEM
               : result == E_NOTIMPL     ? -95
                                         : -EIO;
    } catch (...) {
        handler->_linuxReadResult = E_OUTOFMEMORY;
        return -12;
    }
}

HRESULT CHandler::LoadLinuxTables(UInt64 &directoryEnd) {
    _linuxReadResult = S_OK;
    uint64_t end;
    const int created = sevenzip_squashfs_metadata_create(_h.Size, ReadLinux, ReadLinuxMetadata,
                                                          this, &_linuxMetadata, &end);
    if (created != 0)
        return LinuxResult(created);
    // Ancillary I/O failures have been retained as unavailable tables by the
    // C adapter. A successful subset must not inherit its last tolerated read.
    _linuxReadResult = S_OK;
    const unsigned unavailable = sevenzip_squashfs_unavailable_tables(_linuxMetadata);
    _indexLimitations = unavailable != 0;
    directoryEnd = end;
    _uids.Alloc((size_t)_h.NumIDs * 4);
    return S_OK;
}

HRESULT CHandler::LoadId(UInt32 index) {
    if (index >= _h.NumIDs || _loadedIds.count(index))
        return S_OK;
    _linuxReadResult = S_OK;
    unsigned id = 0;
    const HRESULT result = LinuxResult(sevenzip_squashfs_id(_linuxMetadata, index, &id));
    if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
        return result;
    if (_openCallback) {
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
        RINOK(_directoryCallbackResult)
    }
    if (result != S_OK)
        _unavailableIds.insert(index);
    SetUi32(_uids + (size_t)index * 4, id);
    _loadedIds.insert(index);
    return S_OK;
}

HRESULT CHandler::LoadFragment(CNode &node) {
    if ((node.Type != kType_FILE && node.Type != kType_FILE + 7) || !node.ThereAreFrags())
        return S_OK;
    const auto found = _fragmentIndexes.find(node.Frag);
    if (found != _fragmentIndexes.end()) {
        node.Frag = found->second;
        return S_OK;
    }
    _linuxReadResult = S_OK;
    uint64_t start = 0;
    unsigned size = 0;
    const HRESULT result =
        LinuxResult(sevenzip_squashfs_fragment(_linuxMetadata, node.Frag, &start, &size));
    if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
        return result;
    if (_openCallback) {
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
        RINOK(_directoryCallbackResult)
    }
    CFrag frag;
    const UInt32 packed = GET_COMPRESSED_BLOCK_SIZE(size);
    frag.Available = result == S_OK && packed != 0 && packed <= _h.BlockSize && start <= _h.Size &&
                     packed <= _h.Size - start;
    frag.StartBlock = start;
    frag.Size = size;
    const UInt32 index = _frags.Size();
    _fragmentIndexes.emplace(node.Frag, index);
    _frags.Add(frag);
    node.Frag = index;
    return S_OK;
}

HRESULT CHandler::ReadInodeBytes(void *opaque, UInt64 *block, unsigned *offset, void *buffer,
                                 unsigned size) {
    CHandler *handler = static_cast<CHandler *>(opaque);
    uint64_t position = *block;
    handler->_linuxReadResult = S_OK;
    const UInt64 savedEnd = handler->_metadataReadEnd;
    handler->_metadataReadEnd = handler->_h.DirTable;
    const int code =
        sevenzip_squashfs_inode_read(handler->_linuxMetadata, &position, offset, buffer, size);
    handler->_metadataReadEnd = savedEnd;
    *block = position;
    const HRESULT result = handler->LinuxResult(code);
    if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
        return result;
    if (result != S_OK && handler->_linuxReadResult != S_OK)
        handler->_inodeTableIncomplete = true;
    if (handler->_openCallback && ((handler->_inodeReads++ & 255) == 0 || result != S_OK)) {
        handler->_directoryCallbackResult = handler->_openCallback->SetCompleted(NULL, NULL);
        RINOK(handler->_directoryCallbackResult)
    }
    return result == S_OK ? S_OK : S_FALSE;
}

HRESULT CHandler::ReadInode(UInt64 reference, CNode &node, std::vector<Byte> &link,
                            CInodePosition &end, bool complete) {
    const UInt64 block = reference >> 16;
    if (block > UINT32_MAX || block >= _h.DirTable - _h.InodeTable)
        return S_FALSE;
    CInodeReader reader;
    reader.Context = this;
    reader.Read = ReadInodeBytes;
    reader.Block = _h.InodeTable + block;
    reader.Offset = (unsigned)reference & 0xFFFF;
    UInt32 parsed = node.Parse4(NULL, 0, _h, NULL, true, &reader);
    if (parsed && complete) {
        if (node.Type == kType_FILE || node.Type == kType_FILE + 7) {
            const HRESULT result = reader.Skip(node.GetNumBlocks(_h) * 4);
            if (result != S_OK)
                parsed = 0;
        } else if (node.Type == kType_DIR + 7) {
            parsed = node.Parse4(reader.Data, reader.Size, _h, NULL, false, &reader);
        }
    }
    if (_inodeSnapshot.Size() == 0 && _h.DirTable - _h.InodeTable <= kMetadataBlockSize + 2) {
        Byte snapshot[kMetadataBlockSize];
        const unsigned size =
            sevenzip_squashfs_cached_inodes(_linuxMetadata, snapshot, sizeof(snapshot));
        if (size) {
            _inodeSnapshot.Alloc(size);
            memcpy(_inodeSnapshot, snapshot, size);
        }
    }
    if ((reader.Result == S_OK || reader.Result == S_FALSE) && _openCallback &&
        _directoryCallbackResult == S_OK)
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
    RINOK(_directoryCallbackResult)
    RINOK(reader.Result)
    if (!parsed) {
        _inodeTableIncomplete = true;
        return S_FALSE;
    }
    if (node.IsLink()) {
        const Byte *target = reader.Data + _h.GetSymLinkOffset();
        link.assign(target, target + (size_t)node.FileSize);
    }
    end = CInodePosition(reader.Block, reader.Offset);
    return S_OK;
}

HRESULT CHandler::AddInode(UInt64 reference, const CNode &n, const std::vector<Byte> &link,
                           const CInodePosition &end, int &nodeIndex) {
    const CInodePosition start(_h.InodeTable + (reference >> 16), (unsigned)reference & 0xFFFF);
    auto next = _inodeNodes.lower_bound(reference);
    if (next != _inodeNodes.end() && next->first == reference) {
        nodeIndex = (int)next->second.first;
        next->second.second = MyMax(next->second.second, end);
        ++next;
    } else {
        if (_nodes.Size() >= kNumFilesMax)
            return S_FALSE;
        const UInt16 type = n.Type > kType_SOCK ? n.Type - 7 : n.Type;
        const auto inserted = _inodeReferences.emplace(n.Number, std::make_pair(reference, type));
        if (!inserted.second) {
            inserted.first->second.first = UINT64_MAX;
            _inodeMappingLimitations = true;
        }
        const unsigned index = _nodes.Size();
        _attributes.resize(index + 1);
        _loadedNodes.Add(false);
        _nodeReferences.Add(reference);
        _nodeLinks.push_back(link);
        _nodes.Add(n);
        _activeDirs.Add(false);
        _inodeNodes.emplace_hint(next, reference, std::make_pair(index, end));
        nodeIndex = (int)index;
    }
    if (next != _inodeNodes.end() &&
        end > CInodePosition(_h.InodeTable + (next->first >> 16), (unsigned)next->first & 0xFFFF))
        _inodeTableIncomplete = true;
    auto previous = _inodeNodes.find(reference);
    if (previous != _inodeNodes.begin()) {
        --previous;
        if (previous->second.second > start)
            _inodeTableIncomplete = true;
    }
    if (_openCallback && (_nodes.Size() & 255) == 0) {
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
        RINOK(_directoryCallbackResult)
    }
    return S_OK;
}

HRESULT CHandler::ReadNode(UInt64 reference, int &nodeIndex) {
    nodeIndex = -1;
    const auto found = _inodeNodes.find(reference);
    if (found != _inodeNodes.end()) {
        nodeIndex = (int)found->second.first;
        return S_OK;
    }
    if (_badInodeReferences.count(reference))
        return S_FALSE;
    CNode node;
    std::vector<Byte> link;
    CInodePosition end;
    const HRESULT result = ReadInode(reference, node, link, end, false);
    RINOK(_directoryCallbackResult)
    if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
        return result;
    if (result != S_OK) {
        _badInodeReferences.insert(reference);
        return S_FALSE;
    }
    RINOK(AddInode(reference, node, link, end, nodeIndex))
    return S_OK;
}

HRESULT CHandler::LoadNode(unsigned index) {
    if (_loadedNodes[index])
        return S_OK;
    CNode &n = _nodes[index];
    RINOK(LoadId(n.Uid))
    RINOK(LoadId(n.Gid))
    RINOK(LoadFragment(n))
    _ownershipLimitations |= OwnershipUnavailable(n);
    _fragmentLimitations |= FragmentUnavailable(n);
    _linuxReadResult = S_OK;
    auto found = _attributeSets.find(n.Xattr);
    if (found == _attributeSets.end()) {
        _attributes[index] = std::make_shared<std::vector<ArchiveAttribute>>();
        CAttributeContext context{this, index, S_OK};
        const int result =
            sevenzip_squashfs_visit_xattrs(_linuxMetadata, n.Xattr, EmitAttribute, &context);
        RINOK(context.Result)
        const HRESULT attributeResult = LinuxResult(result);
        if (attributeResult == E_ABORT || attributeResult == E_OUTOFMEMORY)
            return attributeResult;
        if (attributeResult != S_OK)
            _incompleteAttributes.insert(n.Xattr);
        _attributeSets.emplace(n.Xattr, _attributes[index]);
    } else
        _attributes[index] = found->second;
    _loadedNodes[index] = true;
    return S_OK;
}

HRESULT CHandler::CreateLinuxFile(unsigned index, sevenzip_squashfs_file **file) {
    const CNode &node = _nodes[index];
    _linuxReadResult = S_OK;
    const UInt64 savedEnd = _metadataReadEnd;
    _metadataReadEnd = _h.DirTable;
    const int result = sevenzip_squashfs_file_create(
        _h.Size, _h.InodeTable, _h.DirTable, _h.BlockSize, _nodeReferences[index],
        node.Type == kType_FILE ? 32 : 56, node.FileSize, node.GetNumBlocks(_h), node.StartBlock,
        node.Number, ReadLinuxMetadata, this, file);
    _metadataReadEnd = savedEnd;
    return LinuxResult(result);
}

HRESULT CHandler::ReadLinuxFileBlock(sevenzip_squashfs_file *file, UInt64 index, uint64_t *start,
                                     unsigned *size) {
    _linuxReadResult = S_OK;
    const UInt64 savedEnd = _metadataReadEnd;
    _metadataReadEnd = _h.DirTable;
    const int result = sevenzip_squashfs_file_block(file, index, start, size);
    _metadataReadEnd = savedEnd;
    return LinuxResult(result);
}

HRESULT CHandler::GetLinuxPackSize(unsigned index, UInt64 &total) {
    sevenzip_squashfs_file *raw = NULL;
    const HRESULT created = CreateLinuxFile(index, &raw);
    std::unique_ptr<sevenzip_squashfs_file, decltype(&sevenzip_squashfs_file_destroy)> file(
        raw, sevenzip_squashfs_file_destroy);
    if (created == S_FALSE)
        _incompleteBlocklists.insert(index);
    RINOK(created)
    const CNode &node = _nodes[index];
    total = 0;
    for (UInt64 i = 0; i < node.GetNumBlocks(_h); ++i) {
        uint64_t start;
        unsigned size;
        const HRESULT result = ReadLinuxFileBlock(file.get(), i, &start, &size);
        if (result == S_FALSE)
            _incompleteBlocklists.insert(index);
        RINOK(result)
        total += GET_COMPRESSED_BLOCK_SIZE(size);
        if (_openCallback && (i & 255) == 0) {
            _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
            RINOK(_directoryCallbackResult)
        }
    }
    if (node.ThereAreFrags()) {
        if (FragmentUnavailable(node))
            return S_FALSE;
        const CFrag &frag = _frags[node.Frag];
        const UInt32 size = GET_COMPRESSED_BLOCK_SIZE(frag.Size);
        if (size == 0 || size > _h.BlockSize || frag.StartBlock > _h.Size ||
            size > _h.Size - frag.StartBlock)
            return S_FALSE;
        if (node.Offset == 0)
            total += size;
    }
    return S_OK;
}

int CHandler::EmitAttribute(void *opaque, const char *name, const void *value,
                            size_t size) noexcept {
    auto &context = *static_cast<CAttributeContext *>(opaque);
    try {
        if (context.Owner->_openCallback) {
            context.Result = context.Owner->_openCallback->SetCompleted(NULL, NULL);
            context.Owner->_directoryCallbackResult = context.Result;
        }
        if (context.Result == S_OK)
            context.Result = AppendArchiveAttribute(*context.Owner->_attributes[context.Node], name,
                                                    value, size);
    } catch (...) {
        context.Result = E_FAIL;
    }
    return context.Result == S_OK ? 0 : -5;
}

HRESULT CHandler::AddAttributeItems(int root) {
    unsigned count = _items.Size();
    if (!_attributes[(unsigned)root]->empty() || _incompleteDirectories.count(root)) {
        if (_items.Size() >= kNumFilesMax)
            return S_FALSE;
        _rootAttributeName = "[ROOT]";
        for (;;) {
            bool collision = false;
            for (unsigned i = 0; i < count; ++i)
                if (_items[i].Parent < 0 &&
                    StringsAreEqualNoCase(UString(GetPath(i)), UString(_rootAttributeName)))
                    collision = true;
            if (!collision)
                break;
            _rootAttributeName += '_';
        }
        CItem item;
        item.Construct();
        item.Node = root;
        item.Attribute = -2;
        _items.Add(item);
        ++count;
    }
    struct CaseLess {
        bool operator()(const UString &a, const UString &b) const {
            return MyStringCompareNoCase(a, b) < 0;
        }
    };
    std::set<UString, CaseLess> paths;
    for (unsigned i = 0; i < count; ++i)
        paths.insert(MultiByteToUnicodeString(GetPath(i), _openCodePage));
    for (unsigned i = 0; i < count; ++i) {
        const int node = _items[i].Node;
        if (node < 0)
            continue;
        const auto &attributes = *_attributes[(unsigned)node];
        for (unsigned a = 0; a < attributes.size(); ++a) {
            if (_items.Size() >= kNumFilesMax)
                return S_FALSE;
            CItem item;
            item.Construct();
            item.Node = node;
            item.Parent = (int)i;
            item.Attribute = (int)a;
            item.RecoveredInode = _items[i].RecoveredInode;
            const UString prefix = MultiByteToUnicodeString(GetPath(i), _openCodePage);
            for (;;) {
                UString path = prefix;
                path += L':';
                path += attributes[a].name;
                if (item.AttributeSuffix) {
                    path += L'#';
                    path.Add_UInt32(item.AttributeSuffix);
                }
                if (paths.insert(path).second)
                    break;
                ++item.AttributeSuffix;
            }
            _items.Add(item);
        }
    }
    return S_OK;
}

static const Byte kProps[] = {
    kpidPath,  kpidIsAltStream, kpidError,  kpidDeviceMajor, kpidDeviceMinor, kpidIsDir,
    kpidSize,  kpidPackSize,    kpidMTime,  kpidPosixAttrib, kpidUserId,      kpidGroupId,
    kpidINode, kpidLinks,       kpidSymLink
    // kpidLinks,
    // kpidOffset
};

static const PROPID kpidReadLimitations = kpidUserDefined;
static const char kAttributeLimitations[] = "Some SquashFS extended attributes are unavailable. "
                                            "Readable file data and attributes remain accessible.";
static const char kOwnershipLimitations[] =
    "Some SquashFS ownership metadata is unavailable. User or group IDs may be unknown. Readable "
    "file data remains accessible.";
static const char kFragmentLimitations[] =
    "Some SquashFS file fragments are unavailable. Only readable prefixes of affected files are "
    "available; copying them reports an error.";
static const CStatProp kArcProps[] = {
    {NULL, kpidHeadersSize, VT_UI8}, {NULL, kpidFileSystem, VT_BSTR},
    {NULL, kpidMethod, VT_BSTR},     {NULL, kpidClusterSize, VT_UI4},
    {NULL, kpidBigEndian, VT_BOOL},  {NULL, kpidCTime, VT_FILETIME},
    {NULL, kpidCharacts, VT_BSTR},   {NULL, kpidCodePage, VT_BSTR},
    {NULL, kpidReadOnly, VT_BOOL},   {"Read-only limitations", kpidReadLimitations, VT_BSTR}};
IMP_IInArchive_Props IMP_IInArchive_ArcProps_WITH_NAME

    HRESULT
    CHandler::Decompress(ISequentialOutStream *outStream, Byte *outBuf, bool *outBufWasWritten,
                         UInt32 *outBufWasWrittenSize, UInt32 inSize, UInt32 outSizeMax) {
    if (outBuf) {
        *outBufWasWritten = false;
        *outBufWasWrittenSize = 0;
    }
    UInt32 method = _h.Method;
    if (method == kMethod_ZLIB) {
        _zlibDecoder.Create_if_Empty();
        const UInt64 outputLimit = outSizeMax;
        RINOK(_zlibDecoder.Interface()->Code(_limitedInStream, outStream, NULL, &outputLimit, NULL))

    } else {
        if (_inputBuffer.Size() < inSize)
            _inputBuffer.Alloc(inSize);
        RINOK(ReadStream_FALSE(_stream, _inputBuffer, inSize))

        Byte *dest = outBuf;
        if (!outBuf) {
            dest = _dynOutStream->GetBufPtrForWriting(outSizeMax);
            if (!dest)
                return E_OUTOFMEMORY;
        }

        SizeT destLen = outSizeMax, srcLen = inSize;

        if (method == kMethod_LZO || method == kMethod_LZ4) {
            const int result =
                sevenzip_squashfs_decode(_linuxDecoder, _inputBuffer, inSize, dest, outSizeMax);
            if (result < 0)
                return result == -12 ? E_OUTOFMEMORY : S_FALSE;
            destLen = (SizeT)result;
        } else if (method == kMethod_ZSTD) {
            const Byte *src = _inputBuffer;

            if (!_zstd) {
                _zstd = ZstdDec_Create(&g_AlignedAlloc, &g_AlignedAlloc);
                if (!_zstd)
                    return E_OUTOFMEMORY;
            }

            CZstdDecState state;
            ZstdDecState_Clear(&state);

            state.inBuf = src;
            state.inLim = srcLen; //  + 1; for debug
            // state.outStep = outSizeMax;

            state.outBuf_fromCaller = dest;
            state.outBufSize_fromCaller = outSizeMax;
            // state.mustBeFinished = True;

            ZstdDec_Init(_zstd);
            SRes sres;
            for (;;) {
                sres = ZstdDec_Decode(_zstd, &state);
                if (sres != SZ_OK)
                    break;
                if (state.status == ZSTD_STATUS_FINISHED_FRAME)
                    break;
                if (state.inLim == state.inPos && (state.status == ZSTD_STATUS_NEEDS_MORE_INPUT ||
                                                   state.status == ZSTD_STATUS_FINISHED_FRAME))
                    break;
                // sres = sres;
                // break; // for debug
            }

            CZstdDecResInfo info;
            // ZstdDecInfo_Clear(&stat);
            // stat->InSize = state.inPos;
            ZstdDec_GetResInfo(_zstd, &state, sres, &info);
            sres = info.decode_SRes;
            if (sres == SZ_OK) {
                if (state.status != ZSTD_STATUS_FINISHED_FRAME
                    // ||stat.UnexpededEnd
                    || info.extraSize != 0)
                    sres = SZ_ERROR_DATA;
            }
            if (sres != SZ_OK)
                return SResToHRESULT(sres);
            if (state.winPos > outSizeMax)
                return E_FAIL;
            // memcpy(dest, state.dic, state.dicPos);
            destLen = state.winPos;
        } else {
            ECoderStatus status;
            const SRes res =
                XzUnpacker_CodeFullOneStream(&_xz, dest, &destLen, _inputBuffer, &srcLen, &status);
            if (res != 0)
                return SResToHRESULT(res);
            if (status != CODER_STATUS_FINISHED_WITH_MARK || !XzUnpacker_IsStreamWasFinished(&_xz))
                return S_FALSE;
        }

        if (outBuf) {
            *outBufWasWritten = true;
            *outBufWasWrittenSize = (UInt32)destLen;
        } else
            _dynOutStream->UpdateSize(destLen);
    }
    return S_OK;
}

// in  : packSize : allowed packSize limit
// out : packSize : real packSize in block
// out(packSize) <= in(packSize)
HRESULT CHandler::ReadMetadataBlock(UInt32 &packSize, bool *boundaryKnown,
                                    CDynBufSeqOutStream *output) {
    if (boundaryKnown)
        *boundaryKnown = false;
    if (!output)
        output = _dynOutStream.ClsPtr();
    Byte temp[3];
    const unsigned offset = 2;
    if (offset > packSize)
        return S_FALSE;
    RINOK(ReadStream_FALSE(_stream, temp, offset))
    UInt32 size = GetUi16(temp);
    const bool isCompressed = ((size & kNotCompressedBit16) == 0);
    if (size != kNotCompressedBit16)
        size &= ~kNotCompressedBit16;
    if (size == 0 || size > kMetadataBlockSize)
        return S_FALSE;
    const UInt32 packSize2 = offset + size;
    if (packSize2 > packSize)
        return S_FALSE;
    packSize = packSize2;
    if (boundaryKnown)
        *boundaryKnown = true;
    if (isCompressed) {
        const size_t previousSize = output->GetSize();
        Byte *buffer = output->GetBufPtrForWriting(kMetadataBlockSize);
        if (!buffer)
            return E_OUTOFMEMORY;
        bool written = false;
        UInt32 writtenSize = 0;
        _limitedInStream->Init(size);
        RINOK(Decompress(output, buffer, &written, &writtenSize, size, kMetadataBlockSize))
        if (written)
            output->UpdateSize(writtenSize);
        const size_t unpackSize = output->GetSize() - previousSize;
        if (unpackSize == 0 || unpackSize > kMetadataBlockSize)
            return S_FALSE;
    } else {
        // size != 0 here
        Byte *buf = output->GetBufPtrForWriting(size);
        if (!buf)
            return E_OUTOFMEMORY;
        RINOK(ReadStream_FALSE(_stream, buf, size))
        output->UpdateSize(size);
    }
    return S_OK;
}

HRESULT CHandler::ReadDirectoryData(UInt64 end) {
    const UInt64 start = _h.DirTable;
    if (end < start || end > _h.Size)
        return S_FALSE;
    _directoryDataEnd = end;
    return S_OK;
}

HRESULT CHandler::ScanCachedInodes(bool forRecovery) {
    if (_inodeInventoryScanned || _inodeSnapshot.Size() == 0)
        return S_OK;
    const UInt32 end = (UInt32)_inodeSnapshot.Size();
    decltype(_inventoryReferences) references;
    _inodeInventoryScanned = true;
    UInt32 pos = 0;
    for (UInt32 i = 0; i < _h.NumInodes; ++i) {
        // Optional directory indexes are not required by namespace enumeration.
        // Their variable extent matters only when proving a recovery inventory.
        if (!forRecovery && pos < end && end - pos >= 40 &&
            GetUi16(_inodeSnapshot + pos) == kType_DIR + 7 &&
            GetUi16(_inodeSnapshot + pos + 32) != 0) {
            _inodeInventoryScanned = false;
            return S_OK;
        }
        CNode node;
        const UInt32 length = pos < end ? node.Parse4(_inodeSnapshot + pos, end - pos, _h) : 0;
        if (!length) {
            _inodeTableIncomplete = true;
            break;
        }
        std::vector<Byte> link;
        if (node.IsLink()) {
            const Byte *target = _inodeSnapshot + pos + _h.GetSymLinkOffset();
            link.assign(target, target + (size_t)node.FileSize);
        }
        const CInodePosition next = pos + length == end
                                        ? CInodePosition(_h.DirTable, 0)
                                        : CInodePosition(_h.InodeTable, pos + length);
        int index;
        const HRESULT result = AddInode(pos, node, link, next, index);
        RINOK(_directoryCallbackResult)
        if (result == S_FALSE) {
            _inodeTableIncomplete = true;
            break;
        }
        RINOK(result)
        const UInt16 type = node.Type > kType_SOCK ? node.Type - 7 : node.Type;
        const auto inserted = references.emplace(node.Number, std::make_pair(pos, type));
        if (!inserted.second) {
            inserted.first->second.first = UINT64_MAX;
            _inodeMappingLimitations = true;
        }
        pos += length;
    }
    if (pos != end)
        _inodeTableIncomplete = true;
    if (!_inodeTableIncomplete)
        _inventoryReferences.swap(references);
    return S_OK;
}

HRESULT CHandler::EnsureInodeInventory() {
    RINOK(ScanCachedInodes(true))
    if (_inodeInventoryScanned || _inodeTableIncomplete)
        return S_OK;
    decltype(_inventoryReferences) references;
    CInodePosition position(_h.InodeTable, 0);
    for (UInt32 i = 0; i < _h.NumInodes; ++i) {
        if (position.first >= _h.DirTable || position.first - _h.InodeTable > UINT32_MAX) {
            _inodeTableIncomplete = true;
            break;
        }
        const UInt64 reference = ((position.first - _h.InodeTable) << 16) | position.second;
        CNode node;
        std::vector<Byte> link;
        CInodePosition next;
        HRESULT result = ReadInode(reference, node, link, next, true);
        RINOK(_directoryCallbackResult)
        if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
            return result;
        if (result != S_OK || next <= position) {
            _inodeTableIncomplete = true;
            break;
        }
        int index;
        result = AddInode(reference, node, link, next, index);
        RINOK(_directoryCallbackResult)
        if (result == S_FALSE) {
            _inodeTableIncomplete = true;
            break;
        }
        RINOK(result)
        const UInt16 type = node.Type > kType_SOCK ? node.Type - 7 : node.Type;
        const auto inserted = references.emplace(node.Number, std::make_pair(reference, type));
        if (!inserted.second) {
            inserted.first->second.first = UINT64_MAX;
            _inodeMappingLimitations = true;
        }
        position = next;
    }
    if (position != CInodePosition(_h.DirTable, 0))
        _inodeTableIncomplete = true;
    _inodeInventoryScanned = true;
    if (!_inodeTableIncomplete)
        _inventoryReferences.swap(references);
    return S_OK;
}

struct CTempItem {
    UInt32 StartBlock;
    UInt32 Number;
    // UInt32 iNodeNumber1;
    UInt32 Offset;
    // UInt16 iNodeNumber2;
    UInt16 Type;
};

HRESULT CHandler::FindNode(UInt32 startBlock, UInt32 offset, int &nodeIndex) {
    nodeIndex = -1;
    if (offset > 0xFFFF)
        return S_FALSE;
    RINOK(ReadNode(((UInt64)startBlock << 16) | offset, nodeIndex))
    return LoadNode((unsigned)nodeIndex);
}

HRESULT CHandler::OpenDir(int parent, UInt32 startBlock, UInt32 offset, unsigned level,
                          int &nodeIndex) {
    nodeIndex = -1;
    if (level > kNumDirLevelsMax)
        return S_FALSE;
    const HRESULT nodeResult = FindNode(startBlock, offset, nodeIndex);
    RINOK(_directoryCallbackResult)
    RINOK(nodeResult)

    const CNode &n = _nodes[nodeIndex];
    if (!n.IsDir())
        return parent < 0 ? S_FALSE : S_OK;
    if (_activeDirs[nodeIndex])
        return S_FALSE;
    if (n.FileSize == 3)
        return S_OK;
    _activeDirs[nodeIndex] = true;
    CRecordVector<CTempItem> tempItems;
    const HRESULT readResult = ReadDirItems(parent, n, tempItems, _directoryCallbackResult);
    if (_openCallback && _directoryCallbackResult == S_OK &&
        (readResult == S_OK || readResult == S_FALSE))
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
    RINOK(_directoryCallbackResult)
    if (readResult != S_OK) {
        if (readResult != S_FALSE)
            return readResult;
        _incompleteDirectories.insert(nodeIndex);
    }
    const unsigned startItemIndex = _items.Size() - tempItems.Size();
    FOR_VECTOR(i, tempItems) {
        const CTempItem &tempItem = tempItems[i];
        const unsigned index = startItemIndex + i;
        int childNode;
        HRESULT result =
            OpenDir((int)index, tempItem.StartBlock, tempItem.Offset, level + 1, childNode);
        RINOK(_directoryCallbackResult)
        if (result == S_FALSE && childNode < 0 && !_inodeTableIncomplete) {
            RINOK(EnsureInodeInventory())
            RINOK(_directoryCallbackResult)
            const auto found = _inventoryReferences.find(tempItem.Number);
            const auto observed = _inodeReferences.find(tempItem.Number);
            if (_inodeInventoryScanned && !_inodeTableIncomplete &&
                found != _inventoryReferences.end() && found->second.first != UINT64_MAX &&
                (observed == _inodeReferences.end() || observed->second.first != UINT64_MAX) &&
                found->second.second == tempItem.Type) {
                const UInt64 reference = found->second.first;
                _items[index].RecoveredInode = true;
                _inodeMappingLimitations = true;
                _incompleteDirectories.insert(nodeIndex);
                result = OpenDir((int)index, (UInt32)(reference >> 16), (UInt32)reference & 0xFFFF,
                                 level + 1, childNode);
                RINOK(_directoryCallbackResult)
            }
        }
        if (result != S_OK) {
            if (result != S_FALSE)
                return result;
            _incompleteDirectories.insert(nodeIndex);
            if (childNode >= 0)
                _incompleteDirectories.insert(childNode);
            _items[index].UnavailableType = tempItem.Type;
        }
        _items[index].Node = childNode;
    }
    _activeDirs[nodeIndex] = false;
    return S_OK;
}

HRESULT CHandler::ReadDirItems(int parent, const CNode &n, CRecordVector<CTempItem> &tempItems,
                               HRESULT &callbackResult) {
    UInt32 rem = 0;
    const Byte *p = NULL;
    if ((UInt32)n.StartBlock != n.StartBlock)
        return S_FALSE;
    uint64_t directoryBlock = 0;
    unsigned directoryOffset = n.Offset;
    UInt32 reads = 0;
    Byte field[256];

    if (n.FileSize < 3 || n.StartBlock >= _directoryDataEnd - _h.DirTable)
        return S_FALSE;
    directoryBlock = _h.DirTable + n.StartBlock;
    rem = (UInt32)n.FileSize - 3;

    const auto readField = [&](UInt32 size) -> HRESULT {
        if (size > rem || size > sizeof(field))
            return S_FALSE;
        _linuxReadResult = S_OK;
        const UInt64 savedEnd = _metadataReadEnd;
        _metadataReadEnd = _directoryDataEnd;
        const int read = sevenzip_squashfs_directory_read(_linuxMetadata, &directoryBlock,
                                                          &directoryOffset, field, size);
        _metadataReadEnd = savedEnd;
        const HRESULT result = LinuxResult(read);
        if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
            return result;
        if (_openCallback && ((reads++ & 255) == 0 || result != S_OK)) {
            callbackResult = _openCallback->SetCompleted(NULL, NULL);
            RINOK(callbackResult)
        }
        if (result != S_OK)
            return S_FALSE;
        p = field;

        return S_OK;
    };

    AString tempString;

    while (rem != 0) {
        RINOK(readField(12))
        CTempItem tempItem;
        tempItem.Number = 0;
        UInt32 count = GetUi32(p);
        tempItem.StartBlock = GetUi32(p + 4);
        const UInt32 inodeNumber = GetUi32(p + 8);
        p += 12;
        rem -= 12;
        if (count >= 256)
            return S_FALSE;
        count++;

        for (UInt32 i = 0; i < count; i++) {
            if (rem == 0)
                return S_FALSE;

            const UInt32 nameOffset = _h.GetFileNameOffset();
            if (rem < nameOffset)
                return S_FALSE;
            RINOK(readField(nameOffset))

            if (_items.Size() >= kNumFilesMax)
                return S_FALSE;
            if (_openCallback) {
                const UInt64 numFiles = _items.Size();
                if ((numFiles & 0xFFFF) == 0) {
                    callbackResult = _openCallback->SetCompleted(&numFiles, NULL);
                    RINOK(callbackResult)
                }
            }

            CItem item;
            item.Construct();
            item.RecoveredInode = parent >= 0 && _items[parent].RecoveredInode;

            UInt32 size;

            GET_16(0, tempItem.Offset);

            const Int64 number = (Int64)inodeNumber + (Int16)GetUi16(p + 2);
            tempItem.Number = number > 0 && (UInt64)number <= _h.NumInodes ? (UInt32)number : 0;

            // GET_16 (2, tempItem.iNodeNumber2);
            GET_16(4, tempItem.Type);
            GET_16(6, size);

            p += nameOffset;
            rem -= nameOffset;
            size++;
            if (rem < size)
                return S_FALSE;
            RINOK(readField(size))

            if (size > 256 || tempItem.Type > kType_SOCK || (size == 1 && p[0] == '.') ||
                (size == 2 && p[0] == '.' && p[1] == '.'))
                return S_FALSE;
            for (UInt32 j = 0; j < size; j++)
                if (p[j] == 0 || p[j] == '/')
                    return S_FALSE;

            if (_openCodePage == CP_UTF8) {
                tempString.SetFrom_CalcLen((const char *)p, size);
                if (!CheckUTF8_AString(tempString))
                    _openCodePage = CP_OEMCP;
            }

            AString name;
            name.SetFrom_CalcLen((const char *)p, size);
            _itemNames.Add(name);

            p += size;
            rem -= size;
            item.Parent = parent;
            _items.Add(item);
            tempItems.Add(tempItem);
        }
    }

    return S_OK;
}

HRESULT CHandler::Open2(IInStream *inStream) {
    {
        Byte buf[kHeaderSize4];
        RINOK(ReadStream_FALSE(inStream, buf, kHeaderSize4))
        if (!_h.Parse(buf))
            return S_FALSE;
        switch (_h.Method) {
        case kMethod_ZLIB:
            break;
        case kMethod_LZO:
        case kMethod_LZ4:
        case kMethod_XZ:
        case kMethod_ZSTD:
            break;
        default:
            return E_NOTIMPL;
        }
    }

    _stream = inStream;
    UInt64 inputSize;
    RINOK(inStream->Seek(0, STREAM_SEEK_END, &inputSize))
    if (_h.Size > inputSize || (Int64)_h.Size < 0 || _h.Size < kHeaderSize4)
        return S_FALSE;
    RINOK(InitLinuxDecoder())

    UInt64 dirEnd = _h.FragTable;

    RINOK(LoadLinuxTables(dirEnd))

    if (_h.InodeTable >= _h.DirTable || _h.DirTable > _h.Size)
        return S_FALSE;

    const UInt64 absOffset = _h.RootInode >> 16;
    if (absOffset >= ((UInt64)1 << 32))
        return S_FALSE;

    int root;
    const HRESULT result = FindNode((UInt32)absOffset, (UInt32)_h.RootInode & 0xFFFF, root);
    RINOK(_directoryCallbackResult)
    RINOK(result)
    if (!_nodes[root].IsDir())
        return S_FALSE;

    RINOK(ReadDirectoryData(dirEnd))

    _activeDirs.ClearAndReserve(_nodes.Size());
    FOR_VECTOR(i, _nodes)
    _activeDirs.AddInReserved(false);
    int rootNodeIndex;
    RINOK(OpenDir(-1, (UInt32)absOffset, (UInt32)_h.RootInode & 0xFFFF, 0, rootNodeIndex))

    RINOK(ScanCachedInodes())
    RINOK(AddAttributeItems(rootNodeIndex))

    {
        const UInt32 alignSize = 1 << 12;
        Byte buf[alignSize];
        UInt32 rem = (UInt32)(0 - _h.Size) & (alignSize - 1);
        _sizeCalculated = _h.Size;
        if (rem != 0 && rem <= inputSize - _h.Size) {
            HRESULT readResult = Seek2(_h.Size);
            if (readResult == S_OK)
                readResult = ReadStream_FALSE(_stream, buf, rem);
            if (readResult == E_ABORT || readResult == E_OUTOFMEMORY)
                return readResult;
            if (readResult == S_OK) {
                size_t i;
                for (i = 0; i < rem && buf[i] == 0; i++)
                    ;
                if (i == rem)
                    _sizeCalculated = _h.Size + rem;
            }
        }
    }
    return S_OK;
}

const Byte *CHandler::GetItemName(unsigned index, unsigned &size) const {
    const AString &name = _itemNames[index];
    size = name.Len();
    return (const Byte *)name.Ptr();
}

AString CHandler::GetPath(unsigned index) const {
    const CItem &entry = _items[index];
    if (entry.Attribute == -2)
        return _rootAttributeName;
    if (entry.Attribute >= 0) {
        AString path = GetPath((unsigned)entry.Parent);
        path += ':';
        path += UnicodeStringToMultiByte(
            (*_attributes[(unsigned)entry.Node])[(unsigned)entry.Attribute].name, CP_UTF8);
        if (entry.AttributeSuffix) {
            path += '#';
            path.Add_UInt32(entry.AttributeSuffix);
        }
        return path;
    }
    unsigned len = 0;
    const unsigned indexMem = index;
    for (;;) {
        const CItem &item = _items[index];
        unsigned i;
        GetItemName(index, i);
        len += i + 1;
        index = (unsigned)item.Parent;
        if (item.Parent < 0)
            break;
    }
    len--;

    AString path;
    char *dest = path.GetBuf_SetEnd(len) + len;
    index = indexMem;
    for (;;) {
        const CItem &item = _items[index];
        unsigned i;
        const Byte *p = GetItemName(index, i);
        dest -= i;
        memcpy(dest, p, i);
        index = (unsigned)item.Parent;
        if (item.Parent < 0)
            break;
        *(--dest) = CHAR_PATH_SEPARATOR;
    }
    return path;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN {
        Close();
        _limitedInStream->SetStream(stream);
        HRESULT res;
        try {
            _openCallback = callback;
            res = Open2(stream);
        } catch (...) {
            Close();
            throw;
        }
        if (res != S_OK) {
            Close();
            return res;
        }
        _stream = stream;
        _openCallback = NULL;
    }
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close()) {
    sevenzip_squashfs_decoder_destroy(_linuxDecoder);
    _linuxDecoder = NULL;
    sevenzip_squashfs_metadata_destroy(_linuxMetadata);
    _linuxMetadata = NULL;
    sevenzip_squashfs_file_destroy(_linuxFile);
    _linuxFile = NULL;
    _nodeReferences.Clear();
    _nodeLinks.clear();
    _inodeSnapshot.Free();
    _inodeReads = 0;
    _incompleteBlocklists.clear();
    _attributes.clear();
    _attributeSets.clear();
    _incompleteAttributes.clear();
    _unavailableIds.clear();
    _incompleteDirectories.clear();
    _inodeReferences.clear();
    _inventoryReferences.clear();
    _inodeNodes.clear();
    _badInodeReferences.clear();
    _itemNames.Clear();
    _directoryDataEnd = 0;
    _metadataReadEnd = 0;
    _inodeTableIncomplete = false;
    _inodeInventoryScanned = false;
    _inodeMappingLimitations = false;
    _ownershipLimitations = false;
    _fragmentLimitations = false;
    _indexLimitations = false;
    _fragmentIndexes.clear();
    _loadedIds.clear();
    _loadedNodes.Clear();
    _rootAttributeName.Empty();
    _openCodePage = CP_UTF8;
    _sizeCalculated = 0;
    _openCallback = NULL;
    _directoryCallbackResult = S_OK;

    _limitedInStream->ReleaseStream();
    _stream.Release();

    _items.Clear();
    _nodes.Clear();
    _activeDirs.Clear();
    _frags.Clear();

    _uids.Free();

    _cachedBlock.Free();
    ClearCache();

    return S_OK;
}

UInt64 CHandler::GetItemSize(unsigned index) const {
    const CItem &item = _items[index];
    if (item.Node < 0)
        return 0;
    if (item.Attribute >= 0)
        return (*_attributes[(unsigned)item.Node])[(unsigned)item.Attribute].value.size();
    return _nodes[item.Node].GetSize();
}
bool CHandler::ItemIsDir(unsigned index) const {
    if (_items[index].Node < 0)
        return _items[index].UnavailableType == kType_DIR;
    if (_items[index].Attribute >= 0)
        return false;
    return _nodes[_items[index].Node].IsDir();
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *type)) {
    if (index >= _items.Size())
        return E_INVALIDARG;
    *parent = (UInt32)_items[index].Parent;
    *type = _items[index].Attribute >= 0 ? NParentType::kAltStream : NParentType::kDir;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32, BSTR *name, PROPID *id)) {
    *name = NULL;
    *id = 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetRawProp(UInt32, PROPID, const void **data, UInt32 *size, UInt32 *type)) {
    *data = NULL;
    *size = 0;
    *type = 0;
    return S_OK;
}

bool CHandler::GetPackSize(unsigned index, UInt64 &totalPack) {
    totalPack = 0;
    if (_items[index].Node < 0)
        return false;
    if (_items[index].Attribute >= 0) {
        totalPack = GetItemSize(index);
        return true;
    }
    const CItem &item = _items[index];
    const CNode &node = _nodes[item.Node];
    if ((node.Type == kType_FILE || node.Type == kType_FILE + 7) && node.FileSize != 0) {
        const HRESULT result = GetLinuxPackSize((unsigned)item.Node, totalPack);
        if (FAILED(result))
            _linuxReadResult = result;
        return result == S_OK;
    }
    if (node.IsLink() || node.FileSize == 0) {
        totalPack = node.FileSize;
        return true;
    }
    return false;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = _items.Size();
    return S_OK;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NWindows::NCOM::CPropVariant prop;
    switch (propID) {
    case kpidReadOnly:
        prop = true;
        break;
    case kpidReadLimitations: {
        AString reasons;
        if (_indexLimitations)
            reasons = "Some SquashFS metadata indexes are unavailable. File data, ownership or "
                      "extended attributes may be incomplete.";
        if (!_incompleteAttributes.empty()) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += kAttributeLimitations;
        }
        if (_ownershipLimitations) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += kOwnershipLimitations;
        }
        if (_fragmentLimitations) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += kFragmentLimitations;
        }
        if (!_incompleteDirectories.empty()) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS directories could not be read completely. Files or names may "
                       "be missing. Readable files remain accessible.";
        }
        if (!_incompleteBlocklists.empty()) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS file block lists are unavailable. Only readable portions "
                       "of affected files can be copied; incomplete copies report an error.";
        }
        if (_inodeMappingLimitations) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS directory references are unavailable. Files recovered by "
                       "inode number may be misidentified or outdated.";
        }
        if (_inodeTableIncomplete) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS inode records are unavailable. Files or metadata may be "
                       "missing. Inode identities are unverified.";
        }
        if (!reasons.IsEmpty())
            prop = reasons;
        break;
    }
    case kpidIsTree:
        prop = true;
        break;
    case kpidIsAltStream: {
        bool found = false;
        FOR_VECTOR(i, _items) if (_items[i].Attribute >= 0) {
            found = true;
            break;
        }
        prop = found;
        break;
    }
    case kpidINode:
        prop = true;
        break;
    case kpidMethod: {
        char sz[16];
        const char *s;
        s = NULL;
        if (_h.Method < Z7_ARRAY_SIZE(k_Methods))
            s = k_Methods[_h.Method];
        if (!s) {
            ConvertUInt32ToString(_h.Method, sz);
            s = sz;
        }

        prop = s;
        break;
    }
    case kpidFileSystem: {
        AString res("SquashFS");

        res.Add_Space();
        res.Add_UInt32(_h.Major);
        res.Add_Dot();
        res.Add_UInt32(_h.Minor);
        prop = res;
        break;
    }
    case kpidClusterSize:
        prop = _h.BlockSize;
        break;
    case kpidBigEndian:
        prop = false;
        break;
    case kpidCTime:
        if (_h.CTime != 0)
            PropVariant_SetFrom_UnixTime(prop, _h.CTime);
        break;
    case kpidCharacts:
        FLAGS_TO_PROP(k_Flags, _h.Flags, prop);
        break;
    // case kpidNumBlocks: prop = _h.NumFrags; break;
    case kpidPhySize:
        prop = _sizeCalculated;
        break;
    case kpidHeadersSize:
        if (_sizeCalculated >= _h.InodeTable)
            prop = _sizeCalculated - _h.InodeTable;
        break;

    case kpidCodePage: {
        char sz[16];
        const char *name = NULL;
        switch (_openCodePage) {
        case CP_OEMCP:
            name = "OEM";
            break;
        case CP_UTF8:
            name = "UTF-8";
            break;
        }
        if (!name) {
            ConvertUInt32ToString(_openCodePage, sz);
            name = sz;
        }
        prop = name;
        break;
    }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (index >= _items.Size())
        return E_INVALIDARG;
    NWindows::NCOM::CPropVariant prop;
    const CItem &item = _items[index];
    if (item.Node < 0) {
        switch (propID) {
        case kpidPath:
        case kpidName: {
            AString path = GetPath(index);
            if (propID == kpidName)
                path = AString(path.Ptr(path.ReverseFind_PathSepar() + 1));
            prop = MultiByteToUnicodeString(path, _openCodePage);
            break;
        }
        case kpidIsDir:
            prop = ItemIsDir(index);
            break;
        case kpidIsAltStream:
            prop = false;
            break;
        case kpidError:
            prop = "The SquashFS inode is unavailable. File data and metadata could not be read.";
            break;
        }
        prop.Detach(value);
        return S_OK;
    }
    const CNode &node = _nodes[item.Node];
    const bool isDir = ItemIsDir(index);

    if (item.Attribute >= 0) {
        switch (propID) {
        case kpidINode:
        case kpidSymLink:
        case kpidDeviceMajor:
        case kpidDeviceMinor:
            prop.Detach(value);
            return S_OK;
        case kpidStreamId:
            prop = ((UInt64)1 << 63) | index;
            prop.Detach(value);
            return S_OK;
        case kpidLinks:
            prop = (UInt32)1;
            prop.Detach(value);
            return S_OK;
        case kpidPosixAttrib:
            prop = (UInt32)(MY_LIN_S_IFREG | 0444);
            prop.Detach(value);
            return S_OK;
        }
    }

    switch (propID) {
    case kpidDeviceMajor:
    case kpidDeviceMinor:
        if ((node.Type == kType_BLK || node.Type == kType_CHR || node.Type == kType_BLK + 7 ||
             node.Type == kType_CHR + 7)) {
            const UInt32 device = sevenzip_squashfs_decode_device(node.RDev);
            prop = propID == kpidDeviceMajor ? device >> 20 : device & 0xFFFFF;
        }
        break;
    case kpidError: {
        AString reasons;
        if (_incompleteDirectories.count(item.Node))
            reasons = "The SquashFS directory could not be read completely.";
        if (InodeIdentityUnavailable(item)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "The SquashFS inode-number mapping is unverified.";
        }
        if (item.Attribute < 0 && _incompleteAttributes.count(node.Xattr)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS extended attributes are unavailable.";
        }
        if (OwnershipUnavailable(node)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "SquashFS user or group IDs are unavailable.";
        }
        if (item.Attribute < 0 && FragmentUnavailable(node)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons +=
                "The SquashFS file fragment is unavailable. Only the readable prefix is available.";
        }
        if (item.Attribute < 0 && _incompleteBlocklists.count((unsigned)item.Node)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS file block-list metadata is unavailable.";
        }
        if (!reasons.IsEmpty())
            prop = reasons;
        break;
    }
    case kpidIsAltStream:
        prop = item.Attribute >= 0;
        break;
    case kpidName: {
        AString path = GetPath(index);
        UString name;
        if (item.Attribute >= 0) {
            name = (*_attributes[(unsigned)item.Node])[(unsigned)item.Attribute].name;
            if (item.AttributeSuffix) {
                name += L'#';
                name.Add_UInt32(item.AttributeSuffix);
            }
        } else
            MultiByteToUnicodeString2(name, AString(path.Ptr(path.ReverseFind_PathSepar() + 1)),
                                      _openCodePage);
        prop = name;
        break;
    }
    case kpidStreamId:
    case kpidINode:
        if (InodeIdentityUnavailable(item) || _inodeTableIncomplete)
            break;
        prop = node.Number;
        break;
    case kpidLinks:
        prop = node.NumLinks;
        break;
    case kpidSymLink:
        if (node.IsLink() && node.FileSize != 0) {
            AString target;
            target.SetFrom_CalcLen((const char *)_nodeLinks[(unsigned)item.Node].data(),
                                   (unsigned)node.FileSize);
            UString text;
            if (_openCodePage != CP_UTF8 || !ConvertUTF8ToUnicode(target, text))
                MultiByteToUnicodeString2(text, target, _openCodePage);
            prop = text;
        }
        break;
    case kpidPath: {
        AString path(GetPath(index));
        UString s;
        if (_openCodePage == CP_UTF8)
            ConvertUTF8ToUnicode(path, s);
        else
            MultiByteToUnicodeString2(s, path, _openCodePage);
        prop = s;
        break;
    }
    case kpidIsDir:
        prop = isDir;
        break;
    // case kpidOffset: if (!node.IsLink()) prop = (UInt64)node.StartBlock; break;
    case kpidSize:
        if (!isDir)
            prop = GetItemSize(index);
        break;
    case kpidPackSize:
        if (!isDir) {
            UInt64 size;
            _linuxReadResult = S_OK;
            if (GetPackSize(index, size))
                prop = size;
            if (FAILED(_linuxReadResult))
                return _linuxReadResult;
        }
        break;
    case kpidMTime:
        PropVariant_SetFrom_UnixTime(prop, node.MTime);
        break;
    case kpidPosixAttrib: {
        if (node.Type != 0 && node.Type < Z7_ARRAY_SIZE(k_TypeToMode))
            prop = (UInt32)(node.Mode & 0xFFF) | k_TypeToMode[node.Type];
        break;
    }
    case kpidUserId:
    case kpidGroupId: {
        UInt32 id = node.Uid;
        const CByteBuffer *ids = &_uids;
        if (propID == kpidGroupId) {
            id = node.Gid;
        }
        const UInt32 offset = (UInt32)id * 4;
        if (_unavailableIds.count(id))
            break;
        if (offset < ids->Size())
            prop = (UInt32)GetUi32(*ids + offset);
        break;
    }
        /*
        case kpidLinks:
          if (node.Type != kType_FILE)
            prop = node.NumLinks;
          break;
        */
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

class CSquashfsInStream : public CCachedInStream {
    HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) Z7_override;

  public:
    CHandler *Handler;
    CMyComPtr<IInArchive> Owner;
};

HRESULT CSquashfsInStream::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) {
    return Handler->ReadBlock(blockIndex, dest, blockSize);
}

HRESULT CHandler::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) {
    const CNode &node = _nodes[_nodeIndex];
    UInt64 blockOffset;
    UInt32 packBlockSize;
    UInt32 offsetInBlock = 0;
    bool compressed;
    if (_linuxFile && blockIndex < node.GetNumBlocks(_h)) {
        uint64_t start;
        unsigned size;
        RINOK(ReadLinuxFileBlock(_linuxFile, blockIndex, &start, &size))
        blockOffset = start;
        packBlockSize = GET_COMPRESSED_BLOCK_SIZE(size);
        compressed = IS_COMPRESSED_BLOCK(size);
    } else {
        if (!node.ThereAreFrags())
            return S_FALSE;
        if (node.Frag >= _frags.Size())
            return S_FALSE;
        const CFrag &frag = _frags[node.Frag];
        if (!frag.Available)
            return S_FALSE;
        offsetInBlock = node.Offset;
        if (offsetInBlock > _h.BlockSize)
            return S_FALSE;
        blockOffset = frag.StartBlock;
        packBlockSize = GET_COMPRESSED_BLOCK_SIZE(frag.Size);
        compressed = IS_COMPRESSED_BLOCK(frag.Size);
    }

    if (packBlockSize > _h.BlockSize || blockOffset > _h.Size ||
        packBlockSize > _h.Size - blockOffset)
        return S_FALSE;

    if (packBlockSize == 0) {
        // sparse file ???
        memset(dest, 0, blockSize);
        return S_OK;
    }

    if (blockOffset != _cachedBlockStartPos || packBlockSize != _cachedPackBlockSize ||
        compressed != _cachedBlockCompressed) {
        ClearCache();
        RINOK(Seek2(blockOffset))
        _limitedInStream->Init(packBlockSize);

        if (compressed) {
            _outStream->Init((Byte *)_cachedBlock, _h.BlockSize);
            bool outBufWasWritten = false;
            UInt32 outBufWasWrittenSize = 0;
            HRESULT res = Decompress(_outStream, _cachedBlock, &outBufWasWritten,
                                     &outBufWasWrittenSize, packBlockSize, _h.BlockSize);
            RINOK(res)
            if (outBufWasWritten)
                _cachedUnpackBlockSize = outBufWasWrittenSize;
            else
                _cachedUnpackBlockSize = (UInt32)_outStream->GetPos();
        } else {
            if (packBlockSize > _h.BlockSize)
                return S_FALSE;
            RINOK(ReadStream_FALSE(_limitedInStream, _cachedBlock, packBlockSize))
            _cachedUnpackBlockSize = packBlockSize;
        }
        _cachedBlockStartPos = blockOffset;
        _cachedPackBlockSize = packBlockSize;
        _cachedBlockCompressed = compressed;
        _processedPackSize += packBlockSize;
    }

    if (_cachedUnpackBlockSize < offsetInBlock ||
        _cachedUnpackBlockSize - offsetInBlock < blockSize)
        return S_FALSE;
    if (blockSize != 0)
        memcpy(dest, _cachedBlock + offsetInBlock, blockSize);
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode,
                               IArchiveExtractCallback *extractCallback)) {
    COM_TRY_BEGIN
    const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
    if (allFilesMode)
        numItems = _items.Size();
    if (numItems == 0)
        return S_OK;
    UInt64 totalSize = 0;
    UInt32 i;
    for (i = 0; i < numItems; i++) {
        const unsigned index = allFilesMode ? i : indices[i];
        if (index >= _items.Size() || UINT64_MAX - totalSize < GetItemSize(index))
            return E_INVALIDARG;
        totalSize += GetItemSize(index);
    }
    RINOK(extractCallback->SetTotal(totalSize))

    UInt64 totalPackSize;
    totalSize = totalPackSize = 0;

    CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
    lps->Init(extractCallback, false);
    CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

    for (i = 0;; i++) {
        lps->InSize = totalPackSize;
        lps->OutSize = totalSize;
        RINOK(lps->SetCur())
        if (i >= numItems)
            break;

        int res;
        {
            CMyComPtr<ISequentialOutStream> outStream;
            const Int32 askMode =
                testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
            const UInt32 index = allFilesMode ? i : indices[i];
            RINOK(extractCallback->GetStream(index, &outStream, askMode))
            // const Byte *p = _data + item.Offset;

            if (ItemIsDir(index)) {
                RINOK(extractCallback->PrepareOperation(askMode))
                const Int32 result = _items[index].Node < 0 ||
                                             InodeIdentityUnavailable(_items[index]) ||
                                             _incompleteDirectories.count(_items[index].Node)
                                         ? NExtract::NOperationResult::kDataError
                                         : NExtract::NOperationResult::kOK;
                RINOK(extractCallback->SetOperationResult(result))
                continue;
            }
            const UInt64 unpackSize = GetItemSize(index);
            totalSize += unpackSize;

            if (!testMode && !outStream)
                continue;
            RINOK(extractCallback->PrepareOperation(askMode))

            res = NExtract::NOperationResult::kDataError;
            {
                CMyComPtr<ISequentialInStream> inSeqStream;
                CHandler *reader = NULL;
                HRESULT hres = CreateReadStream(index, &inSeqStream, &reader);
                if (hres == E_NOTIMPL) {
                    res = NExtract::NOperationResult::kUnsupportedMethod;
                } else if (hres != S_FALSE) {
                    RINOK(hres)
                    if (!inSeqStream)
                        return E_FAIL;
                    {
                        hres = copyCoder.Interface()->Code(inSeqStream, outStream, NULL, NULL, lps);
                        totalPackSize += reader ? reader->_processedPackSize : copyCoder->TotalSize;
                        if (hres == S_OK) {
                            if (copyCoder->TotalSize == unpackSize)
                                res = NExtract::NOperationResult::kOK;
                        } else if (hres == E_NOTIMPL) {
                            res = NExtract::NOperationResult::kUnsupportedMethod;
                        } else if (hres != S_FALSE) {
                            RINOK(hres)
                        }
                    }
                }
            }
        }
        if (InodeIdentityUnavailable(_items[allFilesMode ? i : indices[i]]) &&
            res == NExtract::NOperationResult::kOK)
            res = NExtract::NOperationResult::kDataError;
        RINOK(extractCallback->SetOperationResult(res))
    }

    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream)) {
    return CreateReadStream(index, stream);
}

HRESULT CHandler::CreateReadStream(UInt32 index, ISequentialInStream **stream,
                                   CHandler **readerResult) {
    COM_TRY_BEGIN
    *stream = NULL;
    if (readerResult)
        *readerResult = NULL;
    if (index >= _items.Size())
        return E_INVALIDARG;

    const CItem &item = _items[index];
    if (item.Node < 0)
        return S_FALSE;
    if (item.Attribute >= 0) {
        const auto &value = (*_attributes[(unsigned)item.Node])[(unsigned)item.Attribute].value;
        Create_BufInStream_WithNewBuffer(value.data(), value.size(), stream);
        return S_OK;
    }
    const CNode &node = _nodes[item.Node];

    if (node.IsDir())
        return E_FAIL;

    if (node.FileSize == 0 || node.IsLink()) {
        if (node.IsLink()) {

            const auto &target = _nodeLinks[(unsigned)item.Node];
            Create_BufInStream_WithNewBuffer(target.data(), target.size(), stream);

        } else
            Create_BufInStream_WithNewBuffer(NULL, 0, stream);
        return S_OK;
    }

    CSquashfsInStream *streamSpec = new CSquashfsInStream;
    CMyComPtr<IInStream> streamTemp = streamSpec;
    // A returned stream owns only its inode, mapping and decoder state.  Other
    // GetStream calls and Close/reopen must not change a reader already in use.
    CHandler *reader = new CHandler;
    streamSpec->Owner = reader;
    streamSpec->Handler = reader;
    reader->_h = _h;
    reader->_nodes.Add(node);
    reader->_nodeIndex = 0;

    if (node.ThereAreFrags()) {
        if (node.Frag < _frags.Size())
            reader->_frags.Add(_frags[node.Frag]);
        else {
            CFrag missing = {};
            missing.Available = false;
            reader->_frags.Add(missing);
        }
        reader->_nodes[0].Frag = 0;
    }
    reader->_stream = _stream;
    reader->_limitedInStream->SetStream(_stream);
    RINOK(reader->InitLinuxDecoder())

    reader->_nodeReferences.Add(_nodeReferences[(unsigned)item.Node]);
    RINOK(reader->CreateLinuxFile(0, &reader->_linuxFile))

    reader->_cachedBlock.Alloc(_h.BlockSize);
    reader->ClearCache();
    unsigned cacheSizeLog = 22;
    if (cacheSizeLog <= _h.BlockSizeLog)
        cacheSizeLog = _h.BlockSizeLog + 1;
    if (!streamSpec->Alloc(_h.BlockSizeLog, cacheSizeLog - _h.BlockSizeLog))
        return E_OUTOFMEMORY;
    streamSpec->Init(node.FileSize);
    // The stream owns this reader; extraction may inspect its counters only
    // while keeping the returned stream alive.
    if (readerResult)
        *readerResult = reader;
    *stream = streamTemp.Detach();

    return S_OK;

    COM_TRY_END
}

static const Byte k_Signature[] = {'h', 's', 'q', 's'};

REGISTER_ARC_I("SquashFS", "squashfs", NULL, 0xD2, k_Signature, 0, 0, NULL)

}
}
