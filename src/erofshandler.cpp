// SPDX-License-Identifier: GPL-2.0-only
// 7-Zip archive presentation for the Linux EROFS C read-side modules.
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "Common/UTFConvert.h"
#include "Windows/PropVariant.h"
#include "Windows/TimeUtils.h"
#include "erofsarchiveattributes.h"
#include "erofsarchivereader.h"
#include "include/linux/kdev_t.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace NArchive::NErofs {
namespace {
constexpr UInt32 typeMask = 0170000, directoryType = 0040000, regularType = 0100000,
                 symlinkType = 0120000;
bool IsDir(const erofs_item &item) { return (item.mode & typeMask) == directoryType; }
bool HasData(const erofs_item &item) {
    const UInt32 type = item.mode & typeMask;
    return type == regularType || type == symlinkType;
}
static bool inline IsEmptyData(const Byte *data, unsigned size) {
    for (unsigned i = 0; i < size; i++)
        if (data[i] != 0)
            return false;
    return true;
}
UString Text(const char *bytes, size_t size) {
    AString raw;
    raw.SetFrom(bytes, unsigned(size));
    UString result;
    if (!ConvertUTF8ToUnicode(raw, result))
        MultiByteToUnicodeString2(result, raw);
    return result;
}
struct Item {
    UString path, symlink;
    erofs_item inode{};
    UInt32 parent = UInt32(-1);
    std::shared_ptr<const ErofsAttribute> attribute;
};
struct Pending {
    UString prefix;
    UInt64 nid;
    UInt32 parent = UInt32(-1);
};

class FileStream final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    CMyComPtr<IInArchive> owner_;
    std::shared_ptr<ErofsArchiveReader> reader_;
    ErofsFile file_;
    UInt64 position_ = 0, size_;

  public:
    FileStream(IInArchive *owner, std::shared_ptr<ErofsArchiveReader> reader, ErofsFile file,
               UInt64 size)
        : owner_(owner), reader_(std::move(reader)), file_(std::move(file)), size_(size) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ >= size_)
            return S_OK;
        if (size > size_ - position_)
            size = UInt32(size_ - position_);
        reader_->ResetTransport();
        size_t count = 0;
        const int error = erofs_pread_file(file_.get(), position_, data, size, &count);
        position_ += count;
        if (reader_->extractCallback)
            reader_->completed += count;
        if (processed)
            *processed = UInt32(count);
        return reader_->Result(error);
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const UInt64 base = origin == STREAM_SEEK_SET   ? 0
                            : origin == STREAM_SEEK_CUR ? position_
                                                        : size_;
        if ((offset < 0 && UInt64(-(offset + 1)) + 1 > base) ||
            (offset >= 0 && UInt64(offset) > UInt64(INT64_MAX) - base))
            return E_INVALIDARG;
        position_ = offset < 0 ? base - (UInt64(-(offset + 1)) + 1) : base + UInt64(offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};
} // namespace

class CHandler final : public IInArchive,
                       public IInArchiveGetStream,
                       public IArchiveGetRawProps,
                       public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_3(IInArchive, IInArchiveGetStream, IArchiveGetRawProps)
    Z7_IFACE_COM7_IMP(IInArchive)
    Z7_IFACE_COM7_IMP(IInArchiveGetStream)
    Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
    std::shared_ptr<ErofsArchiveReader> reader_;
    std::vector<Item> items_;
    struct Tree {
        CHandler &handler;
        const Pending &directory;
        std::vector<Pending> &pending;
        std::set<UInt64> &directories;
        std::set<std::string> names;
        HRESULT result = S_OK;
    };
    static int Emit(void *, const char *, size_t, uint64_t, unsigned int);
    HRESULT AddItem(Tree &, const char *, size_t, UInt64);
    HRESULT ReadTree();
    HRESULT ReadAttributes();
    HRESULT ExtractItem(UInt32, Int32, IArchiveExtractCallback *);
};

int CHandler::Emit(void *context, const char *name, size_t size, uint64_t nid, unsigned int) {
    auto &tree = *static_cast<Tree *>(context);
    try {
        tree.result = tree.handler.AddItem(tree, name, size, nid);
    } catch (const std::bad_alloc &) {
        tree.result = E_OUTOFMEMORY;
    } catch (...) {
        tree.result = E_FAIL;
    }
    return tree.result == S_OK ? 0 : 1;
}

HRESULT CHandler::AddItem(Tree &tree, const char *name, size_t size, UInt64 nid) {
    if ((size == 1 && name[0] == '.') || (size == 2 && !memcmp(name, "..", 2)))
        return S_OK;
    if (!size || size > 255 || items_.size() == UINT32_MAX)
        return S_FALSE;
    for (size_t i = 0; i < size; ++i)
        if (!name[i] || name[i] == '/' || name[i] == '\\' || name[i] == ':')
            return S_FALSE;
    if (!tree.names.emplace(name, size).second)
        return S_FALSE;
    Item item;
    item.parent = tree.directory.parent;
    item.path = tree.directory.prefix;
    if (item.path.Len() > UINT32_MAX - size - 1)
        return S_FALSE;
    item.path += Text(name, size);
    ErofsFile file;
    RINOK(reader_->OpenFile(nid, file))
    erofs_get_file_info(file.get(), &item.inode);
    if (IsDir(item.inode)) {
        if (!tree.directories.insert(nid).second)
            return S_FALSE;
        Pending child{item.path, nid, UInt32(items_.size())};
        child.prefix += L'/';
        tree.pending.push_back(std::move(child));
    } else if ((item.inode.mode & typeMask) == symlinkType) {
        if (!item.inode.size || item.inode.size >= 65536)
            return S_FALSE;
        std::vector<char> target(size_t(item.inode.size));
        size_t processed;
        RINOK(reader_->Result(
            erofs_pread_file(file.get(), 0, target.data(), target.size(), &processed)))
        if (processed != target.size() || memchr(target.data(), 0, target.size()))
            return S_FALSE;
        item.symlink = Text(target.data(), target.size());
    }
    items_.push_back(std::move(item));
    reader_->files = items_.size();
    return S_OK;
}

HRESULT CHandler::ReadTree() {
    std::vector<Pending> pending{{UString(), reader_->info.root_nid}};
    std::set<UInt64> directories{reader_->info.root_nid};
    while (!pending.empty()) {
        Pending directory = std::move(pending.back());
        pending.pop_back();
        ErofsFile file;
        RINOK(reader_->OpenFile(directory.nid, file))
        Tree tree{*this, directory, pending, directories, {}, S_OK};
        const int error = erofs_read_directory(file.get(), &tree, Emit);
        RINOK(tree.result)
        RINOK(reader_->Result(error))
    }
    return S_OK;
}

HRESULT CHandler::ReadAttributes() {
    using Values = std::vector<std::shared_ptr<const ErofsAttribute>>;
    std::map<UInt64, Values> cache;
    const auto load = [&](UInt64 nid, const Values *&values) -> HRESULT {
        auto found = cache.find(nid);
        if (found == cache.end()) {
            ErofsFile file;
            RINOK(reader_->OpenFile(nid, file))
            std::vector<ErofsAttribute> attributes;
            RINOK(ReadErofsAttributes(*reader_, file.get(), attributes))
            Values owned;
            for (auto &attribute : attributes)
                owned.push_back(std::make_shared<ErofsAttribute>(std::move(attribute)));
            found = cache.emplace(nid, std::move(owned)).first;
        }
        values = &found->second;
        return S_OK;
    };
    const auto append = [&](UInt32 parent, const Values &values) -> HRESULT {
        for (const auto &attribute : values) {
            if (items_.size() == UINT32_MAX)
                return S_FALSE;
            Item item;
            item.parent = parent;
            item.attribute = attribute;
            item.path = items_[parent].path;
            item.path += L':';
            item.path += attribute->name;
            item.inode = items_[parent].inode;
            item.inode.mode = regularType | 0444;
            item.inode.size = item.inode.packed_size = attribute->value.size();
            item.inode.nlink = 1;
            items_.push_back(std::move(item));
        }
        return S_OK;
    };
    const UInt32 count = UInt32(items_.size());
    for (UInt32 i = 0; i < count; ++i) {
        const Values *values;
        RINOK(load(items_[i].inode.nid, values))
        RINOK(append(i, *values))
    }
    const Values *root;
    RINOK(load(reader_->info.root_nid, root))
    if (!root->empty()) {
        if (items_.size() == UINT32_MAX)
            return S_FALSE;
        Item item;
        item.path = L"[ROOT]";
        for (;;) {
            bool collision = false;
            for (UInt32 i = 0; i < count; ++i)
                if (items_[i].path.IsEqualTo_NoCase(item.path))
                    collision = true;
            if (!collision)
                break;
            item.path += L'_';
        }
        ErofsFile file;
        RINOK(reader_->OpenFile(reader_->info.root_nid, file))
        erofs_get_file_info(file.get(), &item.inode);
        const UInt32 parent = UInt32(items_.size());
        items_.push_back(std::move(item));
        RINOK(append(parent, *root))
    }
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN
    Close();
    struct Reset {
        CHandler &handler;
        bool accepted = false;
        ~Reset() {
            if (handler.reader_)
                handler.reader_->openCallback = nullptr;
            if (!accepted)
                handler.Close();
        }
    } reset{*this};
    reader_ = std::make_shared<ErofsArchiveReader>();
    RINOK(reader_->Open(stream, callback))
    reader_->openCallback = callback;
    RINOK(ReadTree())
    RINOK(ReadAttributes())
    reset.accepted = true;
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close()) {
    reader_.reset();
    items_.clear();
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *count)) {
    *count = UInt32(items_.size());
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (index >= items_.size())
        return E_INVALIDARG;
    const auto &item = items_[index];
    const auto &inode = item.inode;
    NWindows::NCOM::CPropVariant prop;
    switch (id) {
    case kpidPath:
        prop = item.path;
        break;
    case kpidName:
        prop = item.attribute ? item.attribute->name.Ptr()
                              : item.path.Ptr(item.path.ReverseFind_PathSepar() + 1);
        break;
    case kpidIsAltStream:
        prop = bool(item.attribute);
        break;
    case kpidIsDir:
        prop = IsDir(inode);
        break;
    case kpidSize:
        if (!IsDir(inode))
            prop = UInt64(HasData(inode) ? inode.size : 0);
        break;
    case kpidPackSize:
        if (!IsDir(inode))
            prop = UInt64(HasData(inode) ? inode.packed_size : 0);
        break;
    case kpidPosixAttrib:
        prop = UInt32(inode.mode);
        break;
    case kpidUserId:
        prop = UInt32(inode.uid);
        break;
    case kpidGroupId:
        prop = UInt32(inode.gid);
        break;
    case kpidINode:
        if (!item.attribute)
            prop = UInt64(inode.nid);
        break;
    case kpidStreamId:
        if (item.attribute)
            prop = (UInt64(1) << 63) | index;
        break;
    case kpidLinks:
        prop = UInt32(inode.nlink);
        break;
    case kpidSymLink:
        if (!item.symlink.IsEmpty())
            prop = item.symlink;
        break;
    case kpidDeviceMajor:
    case kpidDeviceMinor:
        if ((inode.mode & typeMask) == 0020000 || (inode.mode & typeMask) == 0060000)
            prop = UInt32(id == kpidDeviceMajor ? MAJOR(inode.rdev) : MINOR(inode.rdev));
        break;
    case kpidMTime: {
        UInt64 ticks;
        if (inode.nsec < 1000000000 &&
            NWindows::NTime::UnixTime64_To_FileTime64(inode.mtime, ticks) &&
            ticks <= UINT64_MAX - inode.nsec / 100) {
            ticks += inode.nsec / 100;
            FILETIME time{UInt32(ticks), UInt32(ticks >> 32)};
            prop = time;
        }
        break;
    }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}
Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID id, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NWindows::NCOM::CPropVariant prop;
    if (reader_) {
        const auto &info = reader_->info;
        switch (id) {
        case kpidId: {
            if (!IsEmptyData(info.uuid, sizeof(info.uuid))) {
                char text[sizeof(info.uuid) * 2 + 2];
                ConvertDataToHex_Lower(text, info.uuid, sizeof(info.uuid));
                prop = text;
            }
            break;
        }
        case kpidPhySize:
            prop = UInt64(info.size);
            break;
        case kpidVolumeName:
            prop = Text(info.name, strlen(info.name));
            break;
        case kpidClusterSize:
            prop = UInt32(info.block_size);
            break;
        case kpidINode:
            prop = true;
            break;
        case kpidIsTree:
            prop = true;
            break;
        case kpidIsAltStream:
            prop = std::any_of(items_.cbegin(), items_.cend(),
                               [](const Item &item) { return bool(item.attribute); });
            break;
        }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream)) {
    COM_TRY_BEGIN
    *stream = nullptr;
    if (!reader_ || index >= items_.size())
        return E_INVALIDARG;
    const auto &item = items_[index];
    if (item.attribute) {
        const auto &value = item.attribute->value;
        Create_BufInStream_WithNewBuffer(value.data(), value.size(), stream);
        return S_OK;
    }
    if (IsDir(item.inode))
        return E_FAIL;
    ErofsFile file;
    reader_->ResetTransport();
    RINOK(reader_->OpenFile(item.inode.nid, file))
    CMyComPtr<IInStream> result =
        new FileStream(this, reader_, std::move(file), HasData(item.inode) ? item.inode.size : 0);
    *stream = result.Detach();
    return S_OK;
    COM_TRY_END
}

HRESULT CHandler::ExtractItem(UInt32 index, Int32 testMode, IArchiveExtractCallback *callback) {
    const auto &item = items_[index];
    const Int32 askMode = testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
    CMyComPtr<ISequentialOutStream> output;
    RINOK(callback->GetStream(index, &output, askMode))
    const UInt64 size = HasData(item.inode) ? item.inode.size : 0;
    if (!testMode && !output && !IsDir(item.inode)) {
        reader_->completed += size;
        return S_OK;
    }
    RINOK(callback->PrepareOperation(askMode))
    Int32 operation = NExtract::NOperationResult::kOK;
    if (!IsDir(item.inode) && size) {
        CMyComPtr<ISequentialInStream> source;
        HRESULT result = GetStream(index, &source);
        if (FAILED(result))
            return result;
        if (result != S_OK)
            operation = NExtract::NOperationResult::kDataError;
        std::array<Byte, 65536> buffer;
        UInt64 processedTotal = 0;
        while (operation == NExtract::NOperationResult::kOK && processedTotal < size) {
            UInt32 count = 0;
            result = source->Read(buffer.data(), UInt32(buffer.size()), &count);
            if (FAILED(result))
                return result;
            if (result != S_OK || !count || count > size - processedTotal) {
                operation = NExtract::NOperationResult::kDataError;
                break;
            }
            if (output)
                RINOK(WriteStream(output, buffer.data(), count))
            processedTotal += count;
            if (item.attribute)
                reader_->completed += count;
        }
    }
    output.Release();
    return callback->SetOperationResult(operation);
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 count, Int32 testMode,
                               IArchiveExtractCallback *callback)) {
    COM_TRY_BEGIN
    if (!reader_)
        return E_FAIL;
    const bool all = count == UInt32(-1);
    if (all)
        count = UInt32(items_.size());
    UInt64 total = 0;
    for (UInt32 i = 0; i < count; ++i) {
        const UInt32 index = all ? i : indices[i];
        if (index >= items_.size())
            return E_INVALIDARG;
        const UInt64 size = HasData(items_[index].inode) ? items_[index].inode.size : 0;
        if (UINT64_MAX - total < size)
            return E_INVALIDARG;
        total += size;
    }
    RINOK(callback->SetTotal(total))
    reader_->completed = 0;
    reader_->ResetTransport();
    reader_->extractCallback = callback;
    struct Reset {
        ErofsArchiveReader &reader;
        ~Reset() { reader.extractCallback = nullptr; }
    } reset{*reader_};
    for (UInt32 i = 0; i < count; ++i) {
        RINOK(callback->SetCompleted(&reader_->completed))
        RINOK(ExtractItem(all ? i : indices[i], testMode, callback))
    }
    return callback->SetCompleted(&reader_->completed);
    COM_TRY_END
}

static const Byte kProps[] = {kpidPath,        kpidIsAltStream, kpidIsDir, kpidSize,
                              kpidPackSize,    kpidPosixAttrib, kpidMTime, kpidUserId,
                              kpidGroupId,     kpidINode,       kpidLinks, kpidSymLink,
                              kpidDeviceMajor, kpidDeviceMinor};
static const Byte kArcProps[] = {kpidVolumeName, kpidClusterSize, kpidId};

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *type)) {
    if (index >= items_.size())
        return E_INVALIDARG;
    *parent = items_[index].parent;
    *type = items_[index].attribute ? NParentType::kAltStream : NParentType::kDir;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32, BSTR *name, PROPID *id)) {
    *name = nullptr;
    *id = 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetRawProp(UInt32, PROPID, const void **data, UInt32 *size, UInt32 *type)) {
    *data = nullptr;
    *size = 0;
    *type = 0;
    return S_OK;
}
IMP_IInArchive_Props IMP_IInArchive_ArcProps static const Byte signature[] = {0xe2, 0xe1, 0xf5,
                                                                              0xe0};
REGISTER_ARC_I("EROFS", "erofs img", nullptr, 0xF2, signature, 1024, 0, nullptr)
} // namespace NArchive::NErofs
