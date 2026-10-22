// CramfsHandler.cpp

#include "StdAfx.h"
#include <memory>
#include <set>

#include "../../../C/7zCrc.h"
#include "../../../C/Alloc.h"
#include "../../../C/CpuArch.h"
#include "../../../C/LzmaDec.h"

#include "../../Common/ComTry.h"
#include "../../Common/MyLinux.h"
#include "../../Common/StringConvert.h"

#include "../../Windows/PropVariantUtils.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"
#include "../Compress/ZlibDecoder.h"
#ifdef SEVENZIP_LINUX_CRAMFS
#include "../../../cramfsreader.h"
#endif

namespace NArchive {
namespace NCramfs {

#if !defined(Z7_EXTERNAL_CODECS) || defined(SEVENZIP_LINUX_CRAMFS)
static const Byte kSignature[] = {4, 0x45, 0x3d, 0xcd, 0x28, 4, 0x28, 0xcd, 0x3d, 0x45};
#endif

static const UInt32 kArcSizeMax = (256 + 16) << 20;
static const UInt32 kNumFilesMax = (1 << 19);
static const unsigned kNumDirLevelsMax = (1 << 8);

static const UInt32 kHeaderSize = 0x40;
static const unsigned kHeaderNameSize = 16;
static const UInt32 kNodeSize = 12;

static const UInt32 kFlag_FsVer2 = (1 << 0);
static const UInt32 kFlag_ExtBlockPointers = (1 << 11);

static const unsigned k_Flags_BlockSize_Shift = 11;
static const unsigned k_Flags_BlockSize_Mask = 7;
static const unsigned k_Flags_Method_Shift = 14;
static const unsigned k_Flags_Method_Mask = 3;

/*
  There is possible collision in flags:
    - Original CramFS writes 0 in method field. But it uses ZLIB.
    - Modified CramFS writes 0 in method field for "NONE" compression?
  How to solve that collision?
*/

#define k_Flags_Method_NONE 0
#define k_Flags_Method_ZLIB 1
#define k_Flags_Method_LZMA 2

static const char * const k_Methods[] =
{
    "Copy"
  , "ZLIB"
  , "LZMA"
  , "Unknown"
};

static const CUInt32PCharPair k_Flags[] = {
    {0, "Ver2"}, {1, "SortedDirs"}, {8, "Holes"}, {9, "WrongSignature"}, {10, "ShiftedRootOffset"}};

static const unsigned kBlockSizeLog = 12;

/*
struct CNode
{
  UInt16 Mode;
  UInt16 Uid;
  UInt32 Size;
  Byte Gid;
  UInt32 NameLen;
  UInt32 Offset;

  void Parse(const Byte *p)
  {
    Mode = GetUi16(p);
    Uid = GetUi16(p + 2);
    Size = Get32(p + 4) & 0xFFFFFF;
    Gid = p[7];
    NameLen = p[8] & 0x3F;
    Offset = Get32(p + 8) >> 6;
  }
};
*/

#define Get32(p) (be ? GetBe32(p) : GetUi32(p))

static UInt32 GetMode(const Byte *p, bool be) { return be ? GetBe16(p) : GetUi16(p); }
static bool IsDir(const Byte *p, bool be) { return MY_LIN_S_ISDIR(GetMode(p, be)); }
static bool IsKnownMode(UInt32 mode) {
    return MY_LIN_S_ISDIR(mode) || MY_LIN_S_ISREG(mode) || MY_LIN_S_ISLNK(mode) ||
           MY_LIN_S_ISCHR(mode) || MY_LIN_S_ISBLK(mode) || MY_LIN_S_ISFIFO(mode) || MY_LIN_S_ISSOCK(mode);
}
static const char *const kDirectoryLimitations =
    "Some CramFS directories could not be read completely. Files or names may be missing. Readable "
    "files remain accessible.";

static UInt32 GetSize(const Byte *p, bool be)
{
  if (be)
    return GetBe32(p + 4) >> 8;
  else
    return GetUi32(p + 4) & 0xFFFFFF;
}

static UInt32 GetNameLen(const Byte *p, bool be)
{
  if (be)
    return (p[8] & 0xFC);
  else
    return ((UInt32)p[8] & (UInt32)0x3F) << 2;
}

static UInt32 GetOffset(const Byte *p, bool be)
{
  if (be)
    return (GetBe32(p + 8) & 0x03FFFFFF) << 2;
  else
    return GetUi32(p + 8) >> 6 << 2;
}

static UInt32 GetDataSize(const Byte *p, bool be) {
    const UInt32 mode = GetMode(p, be);
    return (MY_LIN_S_ISREG(mode) || MY_LIN_S_ISLNK(mode)) && GetOffset(p, be) ? GetSize(p, be) : 0;
}

struct CItem
{
  UInt32 Offset;
  int Parent;
  bool DirectoryIncomplete = false;
};

struct CHeader
{
  bool be;
  UInt32 Size;
  UInt32 Flags;
  // UInt32 Future;
  UInt32 Crc;
  // UInt32 Edition;
  UInt32 NumBlocks;
  UInt32 NumFiles;
  char Name[kHeaderNameSize];

  bool Parse(const Byte *p)
  {
    switch (GetUi32(p))
    {
      case 0x28CD3D45: be = false; break;
      case 0x453DCD28: be = true; break;
      default: return false;
    }
    Size = Get32(p + 4);
    Flags = Get32(p + 8);
    // Future = Get32(p + 0xC);
    Crc = Get32(p + 0x20);
    // Edition = Get32(p + 0x24);
    NumBlocks = Get32(p + 0x28);
    NumFiles = Get32(p + 0x2C);
    memcpy(Name, p + 0x30, kHeaderNameSize);
    return true;
  }

  bool IsVer2() const { return (Flags & kFlag_FsVer2) != 0; }
  unsigned GetBlockSizeShift() const { return (unsigned)(Flags >> k_Flags_BlockSize_Shift) & k_Flags_BlockSize_Mask; }
  unsigned GetMethod() const { return (unsigned)(Flags >> k_Flags_Method_Shift) & k_Flags_Method_Mask; }
};



Z7_CLASS_IMP_CHandler_IInArchive_1(
  IInArchiveGetStream
)
  CRecordVector<CItem> _items;
  CMyComPtr<IInStream> _stream;
  Byte *_data;
  std::shared_ptr<Byte> _dataOwner;
  UInt32 _size;
  UInt32 _loadedSize;
  UInt32 _capacity;
  UInt64 _volumePos;
  UInt32 _headersSize;
  UInt32 _headerOffset;
  Int64 _arcOffset;
  bool _linuxRead;
  bool _crcUnavailable;
  bool _directoryLimitations;
  HRESULT _directoryCallbackResult;
  std::set<UInt32> _activeDirectories;
  IArchiveOpenCallback *_openCallback;

  UInt32 _errorFlags;
  bool _isArc;

  CHeader _h;
  UInt32 _phySize;

  unsigned _method;
  unsigned _blockSizeLog;

  NCompress::NZlib::CDecoder *_zlibDecoderSpec;
  CMyComPtr<ICompressCoder> _zlibDecoder;

  CBufInStream *_inStreamSpec;
  CMyComPtr<ISequentialInStream> _inStream;

  CBufPtrSeqOutStream *_outStreamSpec;
  CMyComPtr<ISequentialOutStream> _outStream;

  HRESULT OpenDir(int parent, UInt32 baseOffsetBase, unsigned level);
  HRESULT ReadDirItems(int parent, UInt32 baseOffset, unsigned level);
  void MarkDirectoryIncomplete(int parent) {
      _directoryLimitations = true;
      if (parent >= 0)
          _items[(unsigned)parent].DirectoryIncomplete = true;
  }
  HRESULT Open2(IInStream *inStream);
  HRESULT EnsureData(UInt32 sourceOffset, UInt32 size, UInt32 &offset);
  HRESULT CheckCrc();
  AString GetPath(unsigned index) const;
  HRESULT GetPackSize(unsigned index, UInt32 &res);
  void Free();

  UInt32 GetNumBlocks(UInt32 size) const {
      return (size + ((UInt32)1 << _blockSizeLog) - 1) >> _blockSizeLog;
  }

  void UpdatePhySize(UInt32 s) {
      if (_phySize < s)
          _phySize = s;
  }

public:
  CHandler() : _data(NULL) { Close(); }
  ~CHandler() { Free(); }
  HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize, UInt32 table, UInt32 numBlocks,
                    UInt32 fileSize);
#ifdef SEVENZIP_LINUX_CRAMFS
  static int InflateLinux(void *, void *, size_t, const void *, size_t);
  static int ReadLinux(void *, UInt32, void *, size_t);
  HRESULT _linuxReadResult;
#endif
  };

  static const Byte kProps[] = {
      kpidPath,   kpidIsDir,   kpidSize,        kpidPackSize,   kpidPosixAttrib,
      kpidUserId, kpidGroupId, kpidDeviceMajor, kpidDeviceMinor, kpidError
      // kpidOffset
  };

  static const PROPID kpidReadLimitations = kpidUserDefined + 1;
  static const CStatProp kArcProps[] = {
      {NULL, kpidVolumeName, VT_BSTR}, {NULL, kpidBigEndian, VT_BOOL},
      {NULL, kpidCharacts, VT_BSTR},   {NULL, kpidClusterSize, VT_UI4},
      {NULL, kpidMethod, VT_BSTR},     {NULL, kpidHeadersSize, VT_UI4},
      {NULL, kpidNumSubFiles, VT_UI4}, {NULL, kpidNumBlocks, VT_UI4},
      {NULL, kpidReadOnly, VT_BOOL},   {"Read-only limitations", kpidReadLimitations, VT_BSTR}};

  IMP_IInArchive_Props IMP_IInArchive_ArcProps_WITH_NAME

      HRESULT
      CHandler::OpenDir(int parent, UInt32 baseOffset, unsigned level) {
      UInt32 directory = 0;
      bool inserted = false;
      if (_linuxRead && baseOffset <= _loadedSize && kNodeSize <= _loadedSize - baseOffset &&
          IsDir(_data + baseOffset, _h.be) && GetSize(_data + baseOffset, _h.be)) {
          directory = GetOffset(_data + baseOffset, _h.be);
          inserted = _activeDirectories.insert(directory).second;
          if (!inserted) {
              MarkDirectoryIncomplete(parent);
              return S_OK;
          }
      }
      struct CDirectoryScope {
          std::set<UInt32> &Active;
          UInt32 Directory;
          bool Inserted;
          CDirectoryScope(std::set<UInt32> &active, UInt32 directory, bool inserted)
              : Active(active), Directory(directory), Inserted(inserted) {}
          ~CDirectoryScope() {
              if (Inserted)
                  Active.erase(Directory);
          }
      } scope = {_activeDirectories, directory, inserted};
      const unsigned startIndex = _items.Size();
      const HRESULT result = ReadDirItems(parent, baseOffset, level);
      RINOK(_directoryCallbackResult)
      if (result != S_OK) {
          if (!_linuxRead || result == E_ABORT || result == E_OUTOFMEMORY)
              return result;
          MarkDirectoryIncomplete(parent);
      }
      const unsigned endIndex = _items.Size();
      for (unsigned i = startIndex; i < endIndex; i++) {
          RINOK(OpenDir((int)i, _items[i].Offset, level + 1))
      }
      return S_OK;
  }

  HRESULT CHandler::ReadDirItems(int parent, UInt32 baseOffset, unsigned level) {
      if (baseOffset > _loadedSize || kNodeSize > _loadedSize - baseOffset)
          return S_FALSE;
      const Byte *p = _data + baseOffset;
      bool be = _h.be;
      const UInt32 mode = GetMode(p, be);
      if (!IsKnownMode(mode))
          return S_FALSE;
      if (!IsDir(p, be))
          return S_OK;
      UInt32 offset = GetOffset(p, be);
      UInt32 size = GetSize(p, be);
      if ((_linuxRead && size == 0) || offset == 0)
          return S_OK;
      UInt32 end = offset + size;
      if (offset < kHeaderSize || offset > _size || (!_linuxRead && end > _size) ||
          level > kNumDirLevelsMax)
          return S_FALSE;
      if (_linuxRead)
          end = MyMin(end, _size);
      UpdatePhySize(end);
      if (end > _headersSize)
          _headersSize = end;

      while (size != 0) {
          if (size < kNodeSize || (UInt32)_items.Size() >= kNumFilesMax)
              return S_FALSE;
          CItem item;
          item.Parent = parent;
          item.Offset = offset;
          if (_linuxRead) {
              RINOK(EnsureData(offset, kNodeSize, item.Offset))
          }
          UInt32 nodeLen = kNodeSize + GetNameLen(_data + item.Offset, be);
          if (size < nodeLen || nodeLen == kNodeSize ||
              !IsKnownMode(GetMode(_data + item.Offset, be)))
              return S_FALSE;
          if (_linuxRead) {
              UInt32 nameOffset;
              RINOK(EnsureData(offset + kNodeSize, nodeLen - kNodeSize, nameOffset))
          }
          const Byte *name = _data + item.Offset + kNodeSize;
          unsigned nameLen = nodeLen - kNodeSize;
          while (nameLen && name[nameLen - 1] == 0)
              --nameLen;
          if (!nameLen || (nameLen == 1 && name[0] == '.') ||
              (nameLen == 2 && name[0] == '.' && name[1] == '.'))
              return S_FALSE;
          for (unsigned n = 0; n < nameLen; ++n)
              if (name[n] == 0 || name[n] == '/' || name[n] == '\\' || name[n] == ':')
                  return S_FALSE;
          _items.Add(item);
          if (_openCallback) {
              const UInt64 files = _items.Size();
              _directoryCallbackResult = _openCallback->SetCompleted(&files, NULL);
              RINOK(_directoryCallbackResult)
          }
          offset += nodeLen;
          size -= nodeLen;
      }

      return S_OK;
  }

  HRESULT CHandler::EnsureData(UInt32 sourceOffset, UInt32 size, UInt32 &offset) {
      // Archive item offsets address retained metadata, not gaps in the source.
      if (sourceOffset > _size || size > _size - sourceOffset || size > kArcSizeMax - _loadedSize)
          return S_FALSE;
      offset = _loadedSize;
      const UInt32 end = offset + size;
      if (end > _capacity) {
          UInt32 capacity = _capacity ? _capacity : (1u << 16);
          while (capacity < end)
              capacity = capacity > kArcSizeMax / 2 ? kArcSizeMax : capacity * 2;
          Byte *data = (Byte *)MidAlloc(capacity);
          if (!data)
              return E_OUTOFMEMORY;
          if (_loadedSize)
              memcpy(data, _data, _loadedSize);
          _dataOwner.reset(data, [](Byte *buffer) { MidFree(buffer); });
          _data = data;
          _capacity = capacity;
      }
      RINOK(InStream_SeekSet(_stream, _volumePos + sourceOffset))
      while (_loadedSize < end) {
          size_t part = end - _loadedSize;
          if (part > (1u << 20))
              part = 1u << 20;
          RINOK(ReadStream_FALSE(_stream, _data + _loadedSize, part))
          _loadedSize += (UInt32)part;
          if (_openCallback) {
              _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
              RINOK(_directoryCallbackResult)
          }
      }
      return S_OK;
  }

  HRESULT CHandler::CheckCrc() {
      const auto unavailable = [this](HRESULT result) -> HRESULT {
          if (result == E_ABORT || result == E_OUTOFMEMORY)
              return result;
          _crcUnavailable = true;
          return S_OK;
      };
      HRESULT result = InStream_SeekSet(_stream, _volumePos + _headerOffset);
      if (result != S_OK)
          return unavailable(result);
      Byte buf[1 << 16];
      UInt32 crc = CRC_INIT_VAL;
      UInt32 pos = _headerOffset;
      while (pos < _size) {
          size_t part = _size - pos;
          if (part > sizeof(buf))
              part = sizeof(buf);
          result = ReadStream_FALSE(_stream, buf, part);
          if (result != S_OK)
              return unavailable(result);
          if (pos == _headerOffset)
              SetUi32(buf + 0x20, 0) crc = CrcUpdate(crc, buf, part);
          pos += (UInt32)part;
          if (_openCallback) {
              const UInt64 bytes = pos;
              RINOK(_openCallback->SetCompleted(NULL, &bytes))
          }
      }
      if (CRC_GET_DIGEST(crc) != _h.Crc)
          _errorFlags = kpv_ErrorFlags_HeadersError;
      return S_OK;
  }

HRESULT CHandler::Open2(IInStream *inStream) {
    UInt64 initialPos;
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &initialPos))
    UInt64 headerPos = initialPos;
    UInt64 volumePos = initialPos == 512 ? 0 : initialPos;
    Byte buf[kHeaderSize + kNodeSize];
    const HRESULT headerResult = ReadStream_FALSE(inStream, buf, sizeof(buf));
    if (headerResult == E_ABORT || headerResult == E_OUTOFMEMORY)
        return headerResult;
    if (headerResult != S_OK || !_h.Parse(buf)) {
        if (initialPos != 0)
            return headerResult == S_OK ? S_FALSE : headerResult;
        headerPos = 512;
        RINOK(InStream_SeekSet(inStream, headerPos))
        RINOK(ReadStream_FALSE(inStream, buf, sizeof(buf)))
        if (!_h.Parse(buf))
            return S_FALSE;
    }
    _headerOffset = (UInt32)(headerPos - volumePos);
    _volumePos = volumePos;
    _arcOffset = (Int64)volumePos;
    const bool be = _h.be;
    if (!IsDir(buf + kHeaderSize, be) || (_h.Flags & ~0xFFFFu))
        return S_FALSE;
    const UInt32 rootOffset = GetOffset(buf + kHeaderSize, be);
    if (rootOffset && !(_h.Flags & (1u << 10)) && rootOffset != 76 && rootOffset != 588)
        return S_FALSE;
    _method = k_Flags_Method_ZLIB;
    _blockSizeLog = kBlockSizeLog;
    _phySize = _headerOffset + sizeof(buf);
    _linuxRead = false;
#ifdef SEVENZIP_LINUX_CRAMFS
    _linuxRead = (_h.Flags & ~0xFFFu) == 0;
#endif
    if (_h.IsVer2()) {
        _method = _h.GetMethod();
        // FIT IT. Now we don't know correct way to work with collision in method field.
        if (_method == k_Flags_Method_NONE)
            _method = k_Flags_Method_ZLIB;
        _blockSizeLog = kBlockSizeLog + (_linuxRead ? 0 : _h.GetBlockSizeShift());
        if (_h.Size < _headerOffset + sizeof(buf) || (!_linuxRead && _h.Size > kArcSizeMax))
            return S_FALSE;
        _phySize = _h.Size;
    } else {
        UInt64 size;
        RINOK(InStream_GetSize_SeekToEnd(inStream, size))
        if (size < volumePos || size - volumePos < _headerOffset + sizeof(buf))
            return S_FALSE;
        size -= volumePos;
        const UInt64 limit = _linuxRead ? (UInt64)0xFFFFFFFFu : kArcSizeMax;
        if (size > limit)
            size = limit;
        _h.Size = (UInt32)size;
    }
    if (_linuxRead) {
        UInt64 available;
        RINOK(InStream_GetSize_SeekToEnd(inStream, available))
        if (available < volumePos)
            return S_FALSE;
        available -= volumePos;
        _size = available < _h.Size ? (UInt32)available : _h.Size;
        UInt32 headerCache;
        RINOK(EnsureData(_headerOffset, sizeof(buf), headerCache))
        if (_h.IsVer2()) {
            if (_size != _h.Size)
                _errorFlags = kpv_ErrorFlags_UnexpectedEnd;
            else {
                RINOK(CheckCrc())
            }
        } else
            _phySize = _size;
        return OpenDir(-1, headerCache + kHeaderSize, 0);
    }
    _data = (Byte *)MidAlloc(_h.Size);
    if (!_data)
        return E_OUTOFMEMORY;
    _dataOwner.reset(_data, [](Byte *buffer) { MidFree(buffer); });
    RINOK(InStream_SeekSet(inStream, volumePos))
    size_t processed = 0;
    while (processed < _h.Size) {
        size_t part = _h.Size - processed;
        if (part > (1u << 20))
            part = 1u << 20;
        RINOK(ReadStream(inStream, _data + processed, &part))
        if (!part)
            break;
        processed += part;
        if (_openCallback) {
            const UInt64 bytes = processed;
            RINOK(_openCallback->SetCompleted(NULL, &bytes))
        }
    }
    if (processed < _headerOffset + sizeof(buf))
        return S_FALSE;
    _size = (UInt32)processed;
    _loadedSize = _size;
    if (!_h.IsVer2() && _linuxRead && (_h.Flags & kFlag_ExtBlockPointers))
        _phySize = _size;
    if (_h.IsVer2()) {
        if (_size != _h.Size)
            _errorFlags = kpv_ErrorFlags_UnexpectedEnd;
        else {
            SetUi32(_data + _headerOffset + 0x20,
                    0) if (CrcCalc(_data + _headerOffset, _h.Size - _headerOffset) != _h.Crc) {
                _errorFlags = kpv_ErrorFlags_HeadersError;
                // _errorMessage = "CRC error";
            }
        }
    }

    RINOK(OpenDir(-1, _headerOffset + kHeaderSize, 0))

    if (!_h.IsVer2() && !(_linuxRead && (_h.Flags & kFlag_ExtBlockPointers))) {
        FOR_VECTOR(i, _items) {
            const CItem &item = _items[i];
            const Byte *p = _data + item.Offset;
            if (IsDir(p, be))
                continue;
            UInt32 offset = GetOffset(p, be);
            if (offset < kHeaderSize)
                continue;
            UInt32 numBlocks = GetNumBlocks(GetDataSize(p, be));
            if (numBlocks == 0)
                continue;
            UInt32 start = offset + numBlocks * 4;
            if (start > _size)
                continue;
            UInt32 end = Get32(_data + start - 4);
            if (end >= start)
                UpdatePhySize(end);
        }

        // Read tailing zeros. Most cramfs archives use 4096-bytes aligned zeros
        const UInt32 kTailSize_MAX = 1 << 12;
        UInt32 endPos = (_phySize + kTailSize_MAX - 1) & ~(kTailSize_MAX - 1);
        if (endPos > _size)
            endPos = _size;
        UInt32 pos;
        for (pos = _phySize; pos < endPos && _data[pos] == 0; pos++)
            ;
        if (pos == endPos)
            _phySize = endPos;
    }
    return S_OK;
}

AString CHandler::GetPath(unsigned index) const
{
  unsigned len = 0;
  unsigned indexMem = index;
  for (;;)
  {
    const CItem &item = _items[index];
    const Byte *p = _data + item.Offset;
    unsigned size = GetNameLen(p, _h.be);
    p += kNodeSize;
    unsigned i;
    for (i = 0; i < size && p[i]; i++);
    len += i + 1;
    index = (unsigned)item.Parent;
    if (item.Parent < 0)
      break;
  }
  len--;

  AString path;
  char *dest = path.GetBuf_SetEnd(len) + len;
  index = indexMem;
  for (;;)
  {
    const CItem &item = _items[index];
    const Byte *p = _data + item.Offset;
    unsigned size = GetNameLen(p, _h.be);
    p += kNodeSize;
    unsigned i;
    for (i = 0; i < size && p[i]; i++);
    dest -= i;
    memcpy(dest, p, i);
    index = (unsigned)item.Parent;
    if (item.Parent < 0)
      break;
    *(--dest) = CHAR_PATH_SEPARATOR;
  }
  return path;
}

HRESULT CHandler::GetPackSize(unsigned index, UInt32 &res) {
    res = 0;
    const CItem &item = _items[index];
    const Byte *p = _data + item.Offset;
    const bool be = _h.be;
    const UInt32 numBlocks = GetNumBlocks(GetDataSize(p, be));
    if (numBlocks == 0)
        return S_OK;
    if (_linuxRead && (_h.Flags & kFlag_ExtBlockPointers))
        return S_FALSE;
    const UInt32 offset = GetOffset(p, be);
    if (offset < kHeaderSize)
        return S_FALSE;
    const UInt32 start = offset + numBlocks * 4;
    if (start > _size)
        return S_FALSE;
    UInt32 end;
#ifdef SEVENZIP_LINUX_CRAMFS
    if (_linuxRead) {
        Byte value[4];
        _linuxReadResult = S_OK;
        if (ReadLinux(this, start - 4, value, sizeof(value)))
            return _linuxReadResult == E_ABORT || _linuxReadResult == E_OUTOFMEMORY
                       ? _linuxReadResult
                       : S_FALSE;
        end = Get32(value);
    } else
#endif
        end = Get32(_data + start - 4);
    if (end < start || end > _size || (_linuxRead && (end & 0xc0000000u)))
        return S_FALSE;
    res = end - start;
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN {
        Close();
        _stream = stream;
        _openCallback = callback;
        struct Cleanup {
            CHandler *handler;
            bool accepted;
            ~Cleanup() {
                handler->_openCallback = NULL;
                if (!accepted)
                    handler->Close();
            }
        } cleanup = {this, false};
        RINOK(Open2(stream))
        _isArc = true;
        _stream = stream;
        cleanup.accepted = true;
    }
    return S_OK;
    COM_TRY_END
}

void CHandler::Free()
{
  _dataOwner.reset();
  _data = NULL;
}

Z7_COM7F_IMF(CHandler::Close())
{
  _isArc = false;
  _openCallback = NULL;
  _headerOffset = 0;
  _arcOffset = 0;
  _linuxRead = false;
  _crcUnavailable = false;
  _directoryLimitations = false;
  _directoryCallbackResult = S_OK;
  _activeDirectories.clear();
  _method = k_Flags_Method_ZLIB;
  _blockSizeLog = kBlockSizeLog;
  _h = CHeader();
  _size = 0;
  _loadedSize = 0;
  _capacity = 0;
  _volumePos = 0;
  _phySize = 0;
  _errorFlags = 0;
  _headersSize = 0;
  _items.Clear();
  _stream.Release();
  Free();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
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
        if (_crcUnavailable)
            reasons = "The CramFS volume checksum could not be verified. Some files may be damaged "
                      "or unreadable.";
        else if (_errorFlags & kpv_ErrorFlags_HeadersError)
            reasons = "The CramFS volume checksum does not match. Some files may be damaged.";
        if (_errorFlags & kpv_ErrorFlags_UnexpectedEnd) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "The CramFS source is shorter than the declared volume. Some files may be "
                       "incomplete or unreadable.";
        }
        if (_directoryLimitations) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += kDirectoryLimitations;
        }
        if (!reasons.IsEmpty())
            prop = reasons;
        break;
    }
    case kpidWarning: {
        AString warning;
        if (_crcUnavailable)
            warning =
                "The CramFS volume checksum could not be verified. Some files may be damaged or "
                "unreadable.";
        if (_directoryLimitations) {
            if (!warning.IsEmpty())
                warning += '\n';
            warning += kDirectoryLimitations;
        }
        if (!warning.IsEmpty())
            prop = warning;
        break;
    }
    case kpidVolumeName: {
        char dest[kHeaderNameSize + 4];
        memcpy(dest, _h.Name, kHeaderNameSize);
        dest[kHeaderNameSize] = 0;
        prop = dest;
        break;
    }
    case kpidBigEndian:
        prop = _h.be;
        break;
    case kpidCharacts:
        FLAGS_TO_PROP(k_Flags, _h.Flags, prop);
        break;
    case kpidMethod:
        prop = k_Methods[_method];
        break;
    case kpidClusterSize:
        prop = (UInt32)1 << _blockSizeLog;
        break;
    case kpidNumBlocks:
        if (_h.IsVer2())
            prop = _h.NumBlocks;
        break;
    case kpidNumSubFiles:
        if (_h.IsVer2())
            prop = _h.NumFiles;
        break;
    case kpidPhySize:
        prop = _phySize;
        break;
    case kpidOffset:
        prop.Set_Int64(_arcOffset);
        break;
    case kpidHeadersSize:
        prop = _headersSize;
        break;
    case kpidWarningFlags:
        if (_errorFlags)
            prop = _errorFlags;
        break;
    case kpidErrorFlags: {
        prop = (UInt32)(_isArc ? 0 : kpv_ErrorFlags_IsNotArc);
        break;
    }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  if (index >= _items.Size())
      return E_INVALIDARG;
  NWindows::NCOM::CPropVariant prop;
  const CItem &item = _items[index];
  const Byte *p = _data + item.Offset;
  bool be = _h.be;
  bool isDir = IsDir(p, be);
  switch (propID)
  {
  case kpidError:
      if (item.DirectoryIncomplete)
          prop = "The CramFS directory could not be read completely.";
      break;
  case kpidPath:
      prop = MultiByteToUnicodeString(GetPath(index), CP_UTF8);
      break;
  case kpidIsDir:
      prop = isDir;
      break;
  // case kpidOffset: prop = (UInt32)GetOffset(p, be); break;
  case kpidSize:
      if (!isDir)
          prop = GetDataSize(p, be);
      break;
  case kpidPackSize:
      if (!isDir) {
          UInt32 size;
          const HRESULT result = GetPackSize(index, size);
          if (result == S_OK)
              prop = size;
          else if (result != S_FALSE)
              return result;
      }
      break;
  case kpidPosixAttrib:
      prop = (UInt32)GetMode(p, be);
      break;
  case kpidUserId:
      prop = (UInt32)(be ? GetBe16(p + 2) : GetUi16(p + 2));
      break;
  case kpidGroupId:
      prop = (UInt32)p[7];
      break;
  case kpidDeviceMajor:
      if (MY_LIN_S_ISCHR(GetMode(p, be)) || MY_LIN_S_ISBLK(GetMode(p, be)))
          prop = (UInt32)((GetSize(p, be) >> 8) & 0xFF);
      break;
  case kpidDeviceMinor:
      if (MY_LIN_S_ISCHR(GetMode(p, be)) || MY_LIN_S_ISBLK(GetMode(p, be)))
          prop = (UInt32)(GetSize(p, be) & 0xFF);
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CCramfsInStream: public CCachedInStream
{
  HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) Z7_override;
public:
  CHandler *Handler;
  CMyComPtr<IInArchive> Owner;
  UInt32 Table, NumBlocks, FileSize;
};

HRESULT CCramfsInStream::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize)
{
    return Handler->ReadBlock(blockIndex, dest, blockSize, Table, NumBlocks, FileSize);
}

#ifdef SEVENZIP_LINUX_CRAMFS
int CHandler::ReadLinux(void *context, UInt32 offset, void *dest, size_t size) {
    CHandler *self = static_cast<CHandler *>(context);
    if (self->_linuxReadResult != S_OK)
        return -1;
    self->_linuxReadResult = InStream_SeekSet(self->_stream, self->_volumePos + offset);
    if (self->_linuxReadResult == S_OK)
        self->_linuxReadResult = ReadStream_FALSE(self->_stream, dest, size);
    return self->_linuxReadResult == S_OK ? 0 : -1;
}

int CHandler::InflateLinux(void *context, void *dest, size_t capacity, const void *source,
                           size_t size) {
    CHandler *self = static_cast<CHandler *>(context);
    if (self->_linuxReadResult != S_OK)
        return -1;
    try {
        if (!self->_inStream) {
            self->_inStreamSpec = new CBufInStream();
            self->_inStream = self->_inStreamSpec;
        }
        if (!self->_outStream) {
            self->_outStreamSpec = new CBufPtrSeqOutStream();
            self->_outStream = self->_outStreamSpec;
        }
        self->_inStreamSpec->Init(static_cast<const Byte *>(source), size);
        self->_outStreamSpec->Init(static_cast<Byte *>(dest), capacity);
        self->_linuxReadResult =
            self->_zlibDecoder->Code(self->_inStream, self->_outStream, NULL, NULL, NULL);
        return self->_linuxReadResult == S_OK ? (int)self->_outStreamSpec->GetPos() : -1;
    } catch (...) {
        self->_linuxReadResult = E_OUTOFMEMORY;
        return -1;
    }
}
#endif

HRESULT CHandler::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize, UInt32 table,
                            UInt32 numBlocks, UInt32 fileSize) {
    COM_TRY_BEGIN(void) fileSize;
    if (!_data && !_linuxRead)
        return E_FAIL;
    if (_method == k_Flags_Method_ZLIB) {
        if (!_zlibDecoder) {
            _zlibDecoderSpec = new NCompress::NZlib::CDecoder();
            _zlibDecoder = _zlibDecoderSpec;
        }
    } else {
        if (_method != k_Flags_Method_LZMA) {
            // probably we must support no-compression archives here.
            return E_NOTIMPL;
        }
    }

#ifdef SEVENZIP_LINUX_CRAMFS
    if (_linuxRead) {
        Byte page[4096];
        cramfs_source source = {NULL, _size, _h.be, this, InflateLinux, ReadLinux};
        _linuxReadResult = S_OK;
        if (cramfs_read_block(&source, table, fileSize, (UInt32)blockIndex, page))
            return _linuxReadResult == E_ABORT || _linuxReadResult == E_OUTOFMEMORY ||
                           _linuxReadResult == E_NOTIMPL
                       ? _linuxReadResult
                       : S_FALSE;
        memcpy(dest, page, blockSize);
        return S_OK;
    }
#endif

    const bool be = _h.be;
    const Byte *p2 = _data + (table + (UInt32)blockIndex * 4);
    const UInt32 start = (blockIndex == 0 ? table + numBlocks * 4 : Get32(p2 - 4));
    const UInt32 end = Get32(p2);
    if (end < start || end > _size)
        return S_FALSE;
    const UInt32 inSize = end - start;

    // As in SquashfsHandler::ReadBlock, an empty physical extent is a hole.
    if (inSize == 0) {
        memset(dest, 0, blockSize);
        return S_OK;
    }

    if (_method == k_Flags_Method_LZMA) {
        const unsigned kLzmaHeaderSize = LZMA_PROPS_SIZE + 4;
        if (inSize < kLzmaHeaderSize)
            return S_FALSE;
        const Byte *p = _data + start;
        UInt32 destSize32 = GetUi32(p + LZMA_PROPS_SIZE);
        if (destSize32 > blockSize)
            return S_FALSE;
        SizeT destLen = destSize32;
        SizeT srcLen = inSize - kLzmaHeaderSize;
        ELzmaStatus status;
        SRes res = LzmaDecode(dest, &destLen, p + kLzmaHeaderSize, &srcLen, p, LZMA_PROPS_SIZE,
                              LZMA_FINISH_END, &status, &g_Alloc);
        if (res != SZ_OK ||
            (status != LZMA_STATUS_FINISHED_WITH_MARK &&
             status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK) ||
            destLen != destSize32 || srcLen != inSize - kLzmaHeaderSize)
            return S_FALSE;
        return S_OK;
    }

    if (!_inStream) {
        _inStreamSpec = new CBufInStream();
        _inStream = _inStreamSpec;
    }
    if (!_outStream) {
        _outStreamSpec = new CBufPtrSeqOutStream();
        _outStream = _outStreamSpec;
    }
    _inStreamSpec->Init(_data + start, inSize);
    _outStreamSpec->Init(dest, blockSize);
    RINOK(_zlibDecoder->Code(_inStream, _outStream, NULL, NULL, NULL))
    return (inSize == _zlibDecoderSpec->GetInputProcessedSize() &&
            _outStreamSpec->GetPos() == blockSize)
               ? S_OK
               : S_FALSE;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _items.Size();
  if (numItems == 0)
    return S_OK;
  bool be = _h.be;
  UInt64 totalSize = 0;
  for (UInt32 n = 0; n < numItems; ++n)
      if ((allFilesMode ? n : indices[n]) >= _items.Size())
          return E_INVALIDARG;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const Byte *p = _data + _items[allFilesMode ? i : indices[i]].Offset;
    if (!IsDir(p, be))
        totalSize += GetDataSize(p, be);
  }
  RINOK(extractCallback->SetTotal(totalSize))

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (i = 0; i < numItems; i++) {
      lps->InSize = totalPackSize;
      lps->OutSize = totalSize;
      RINOK(lps->SetCur())
      CMyComPtr<ISequentialOutStream> outStream;
      const Int32 askMode = testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
      const UInt32 index = allFilesMode ? i : indices[i];
      const CItem &item = _items[index];
      RINOK(extractCallback->GetStream(index, &outStream, askMode))
      const Byte *p = _data + item.Offset;

      if (IsDir(p, be)) {
          RINOK(extractCallback->PrepareOperation(askMode))
          RINOK(extractCallback->SetOperationResult(item.DirectoryIncomplete
                                                        ? NExtract::NOperationResult::kDataError
                                                        : NExtract::NOperationResult::kOK))
          continue;
      }
      UInt32 curSize = GetDataSize(p, be);
      totalSize += curSize;
      UInt32 packSize;
      const HRESULT packResult = GetPackSize(index, packSize);
      if (packResult == S_OK)
          totalPackSize += packSize;
      else if (packResult != S_FALSE)
          return packResult;

      if (!testMode && !outStream)
          continue;
      RINOK(extractCallback->PrepareOperation(askMode))

      UInt32 offset = GetOffset(p, be);
      if (offset < kHeaderSize)
          curSize = 0;

      int res = NExtract::NOperationResult::kDataError;
      {
          CMyComPtr<ISequentialInStream> inSeqStream;
          HRESULT hres = GetStream(index, &inSeqStream);
          if (hres == E_NOTIMPL)
              res = NExtract::NOperationResult::kUnsupportedMethod;
          else if (hres != S_FALSE) {
              RINOK(hres)
              if (inSeqStream) {
                  hres = copyCoder->Code(inSeqStream, outStream, NULL, NULL, progress);
                  if (hres == S_OK) {
                      if (copyCoderSpec->TotalSize == curSize)
                          res = NExtract::NOperationResult::kOK;
                  } else if (hres == E_NOTIMPL)
                      res = NExtract::NOperationResult::kUnsupportedMethod;
                  else if (hres != S_FALSE)
                      return hres;
              }
          }
      }
      RINOK(extractCallback->SetOperationResult(res))
  }

  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  if (index >= _items.Size())
      return E_INVALIDARG;

  const CItem &item = _items[index];
  const Byte *p = _data + item.Offset;

  bool be = _h.be;
  if (IsDir(p, be))
    return E_FAIL;

  UInt32 size = GetDataSize(p, be);
  UInt32 numBlocks = GetNumBlocks(size);
  UInt32 offset = GetOffset(p, be);
  if (size == 0 || offset < kHeaderSize) {
      if (size != 0 && offset != 0)
          return S_FALSE;
      CBufInStream *streamSpec = new CBufInStream;
      CMyComPtr<IInStream> streamTemp = streamSpec;
      streamSpec->Init(NULL, 0);
      *stream = streamTemp.Detach();
      return S_OK;
  }

  if (offset + numBlocks * 4 > _size)
      return S_FALSE;
  UInt32 prev = offset;
  for (UInt32 i = 0; !_linuxRead && i < numBlocks; i++) {
      UInt32 next = Get32(_data + offset + i * 4);
      if (next < prev || next > _size)
          return S_FALSE;
      prev = next;
  }

  CCramfsInStream *streamSpec = new CCramfsInStream;
  CMyComPtr<IInStream> streamTemp = streamSpec;
  // Keep the source and decoding state private to the returned stream.
  // Close/reopen of the archive must not change an existing file reader.
  CHandler *reader = new CHandler;
  streamSpec->Owner = reader;
  streamSpec->Handler = reader;
  reader->_h = _h;
  reader->_size = _size;
  reader->_method = _method;
  reader->_blockSizeLog = _blockSizeLog;
  reader->_linuxRead = _linuxRead;
  reader->_volumePos = _volumePos;
  reader->_stream = _stream;
  if (!_linuxRead) {
      reader->_dataOwner = _dataOwner;
      reader->_data = _data;
      reader->_loadedSize = reader->_capacity = _size;
  }
  streamSpec->Table = offset;
  streamSpec->NumBlocks = numBlocks;
  streamSpec->FileSize = size;
  if (!streamSpec->Alloc(_blockSizeLog, 21 - _blockSizeLog))
      return E_OUTOFMEMORY;
  streamSpec->Init(size);
  *stream = streamTemp.Detach();

  return S_OK;
  COM_TRY_END
}

#if !defined(Z7_EXTERNAL_CODECS) || defined(SEVENZIP_LINUX_CRAMFS)
REGISTER_ARC_I("CramFS", "cramfs", NULL, 0xD3, kSignature, 0,
               NArcInfoFlags::kUseGlobalOffset | NArcInfoFlags::kMultiSignature, NULL)
#endif
}}
