// SPDX-License-Identifier: GPL-3.0-or-later
// Local 7-Zip archive ABI presentation for the upstream GRUB XFS C reader.
#include "7zip/Archive/IArchive.h"
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/ComTry.h"
#include "Common/MyCom.h"
#include "Common/UTFConvert.h"
#include "Windows/PropVariant.h"
#include "Windows/TimeUtils.h"
#include "xfsarchivereader.h"
#include <array>
#include <set>
#include <string>
#include <vector>

namespace NArchive::NXfs {
namespace {
bool Directory(const xfs_entry &e) { return (e.mode & 0170000) == 0040000; }
bool Symlink(const xfs_entry &e) { return (e.mode & 0170000) == 0120000; }
bool Regular(const xfs_entry &e) { return (e.mode & 0170000) == 0100000; }
struct Item {
    UString path;
    xfs_entry entry;
};
struct Pending {
    UString prefix;
    UInt64 inode;
    unsigned depth;
};
class FileStream final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    CMyComPtr<IInArchive> owner_;
    std::shared_ptr<XfsArchiveReader> reader_;
    xfs_entry entry_;
    UInt64 position_ = 0;

  public:
    FileStream(IInArchive *owner, std::shared_ptr<XfsArchiveReader> reader, xfs_entry entry)
        : owner_(owner), reader_(std::move(reader)), entry_(entry) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ >= entry_.size || !size)
            return S_OK;
        size_t count = 0;
        const HRESULT result = reader_->Read(entry_.inode, position_, data, size, &count);
        position_ += count;
        if (processed)
            *processed = UInt32(count);
        return result == S_FALSE ? E_FAIL : result;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const UInt64 base = origin == STREAM_SEEK_SET   ? 0
                            : origin == STREAM_SEEK_CUR ? position_
                                                        : entry_.size;
        const UInt64 magnitude = offset < 0 ? UInt64(-(offset + 1)) + 1 : UInt64(offset);
        if ((offset < 0 && magnitude > base) ||
            (offset >= 0 && magnitude > UInt64(INT64_MAX) - base))
            return E_INVALIDARG;
        position_ = offset < 0 ? base - magnitude : base + magnitude;
        if (position)
            *position = position_;
        return S_OK;
    }
};
} // namespace

class CHandler final : public IInArchive, public IInArchiveGetStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IInArchive, IInArchiveGetStream)
    Z7_IFACE_COM7_IMP(IInArchive)
    Z7_IFACE_COM7_IMP(IInArchiveGetStream)
    std::shared_ptr<XfsArchiveReader> reader_;
    std::vector<Item> items_;
    struct Tree {
        CHandler &handler;
        const Pending &directory;
        std::vector<Pending> &pending;
        std::set<UInt64> &directories;
        std::set<std::string> names;
        HRESULT result = S_OK;
    };
    static int Visit(void *, const char *, const xfs_entry *);
    HRESULT Add(Tree &, const char *, const xfs_entry &);
    HRESULT ReadTree(IArchiveOpenCallback *);
    HRESULT ExtractItem(UInt32, Int32, IArchiveExtractCallback *);
};

int CHandler::Visit(void *context, const char *name, const xfs_entry *entry) {
    auto &tree = *static_cast<Tree *>(context);
    try {
        tree.result = tree.handler.Add(tree, name, *entry);
    } catch (const std::bad_alloc &) {
        tree.result = E_OUTOFMEMORY;
    } catch (...) {
        tree.result = E_FAIL;
    }
    return tree.result == S_OK ? 0 : 1;
}
HRESULT CHandler::Add(Tree &tree, const char *bytes, const xfs_entry &entry) {
    if (entry.size > UInt64(INT64_MAX) || items_.size() == UINT32_MAX)
        return S_FALSE;
    for (const auto *p = reinterpret_cast<const unsigned char *>(bytes); *p; ++p)
        if (*p < 32 || *p == '/' || *p == '\\')
            return S_FALSE;
    UString name;
    if (!*bytes || !ConvertUTF8ToUnicode(AString(bytes), name) || !tree.names.insert(bytes).second)
        return S_FALSE;
    Item item{tree.directory.prefix, entry};
    item.path += name;
    if (Directory(entry)) {
        if (tree.directory.depth >= 256 || !tree.directories.insert(entry.inode).second)
            return S_FALSE;
        Pending child{item.path, entry.inode, tree.directory.depth + 1};
        child.prefix += L'/';
        tree.pending.push_back(std::move(child));
    }
    items_.push_back(std::move(item));
    reader_->files = items_.size();
    return S_OK;
}
HRESULT CHandler::ReadTree(IArchiveOpenCallback *callback) {
    reader_->opening = callback;
    struct Reset {
        XfsArchiveReader &reader;
        ~Reset() { reader.opening = nullptr; }
    } reset{*reader_};
    std::vector<Pending> pending{{{}, xfs_root(reader_->volume.get()), 0}};
    std::set<UInt64> directories{xfs_root(reader_->volume.get())};
    while (!pending.empty()) {
        Pending directory = std::move(pending.back());
        pending.pop_back();
        Tree tree{*this, directory, pending, directories, {}};
        const HRESULT result = reader_->List(directory.inode, Visit, &tree);
        if (tree.result != S_OK)
            return tree.result;
        RINOK(result)
        if (callback)
            RINOK(callback->SetCompleted(&reader_->files, nullptr))
    }
    return S_OK;
}
Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN
    Close();
    struct Cleanup {
        CHandler &handler;
        bool accepted = false;
        ~Cleanup() {
            if (!accepted)
                handler.Close();
        }
    } cleanup{*this};
    reader_ = std::make_shared<XfsArchiveReader>(stream);
    RINOK(reader_->Open(callback))
    RINOK(ReadTree(callback))
    cleanup.accepted = true;
    return S_OK;
    COM_TRY_END
}
Z7_COM7F_IMF(CHandler::Close()) {
    items_.clear();
    reader_.reset();
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
    NWindows::NCOM::CPropVariant prop;
    switch (id) {
    case kpidPath:
        prop = item.path;
        break;
    case kpidIsDir:
        prop = Directory(item.entry);
        break;
    case kpidSize:
        if (!Directory(item.entry))
            prop = UInt64(item.entry.size);
        break;
    case kpidPosixAttrib:
        prop = UInt32(item.entry.mode);
        break;
    case kpidINode:
        prop = UInt64(item.entry.inode);
        break;
    case kpidMTime: {
        UInt64 ticks;
        if (NWindows::NTime::UnixTime64_To_FileTime64(item.entry.mtime, ticks)) {
            FILETIME time{UInt32(ticks), UInt32(ticks >> 32)};
            prop = time;
        }
        break;
    }
    case kpidSymLink:
        if (Symlink(item.entry)) {
            if (item.entry.size > 1024)
                return S_FALSE;
            std::array<char, 1025> target{};
            size_t count = 0;
            RINOK(reader_->Read(item.entry.inode, 0, target.data(), item.entry.size, &count))
            if (count != item.entry.size || memchr(target.data(), 0, count))
                return S_FALSE;
            UString text;
            if (!ConvertUTF8ToUnicode(AString(target.data()), text))
                return S_FALSE;
            prop = text;
        }
        break;
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}
Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID id, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    if (reader_) {
        if (id == kpidPhySize)
            prop = UInt64(xfs_size(reader_->volume.get()));
        else if (id == kpidClusterSize)
            prop = UInt32(xfs_block_size(reader_->volume.get()));
        else if (id == kpidReadOnly)
            prop = true;
    }
    prop.Detach(value);
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream)) {
    COM_TRY_BEGIN
    *stream = nullptr;
    if (index >= items_.size())
        return E_INVALIDARG;
    const auto &entry = items_[index].entry;
    if (!Regular(entry) && !Symlink(entry))
        return S_FALSE;
    CMyComPtr<IInStream> result = new FileStream(this, reader_, entry);
    *stream = result.Detach();
    return S_OK;
    COM_TRY_END
}
HRESULT CHandler::ExtractItem(UInt32 index, Int32 testMode, IArchiveExtractCallback *callback) {
    const auto &entry = items_[index].entry;
    const Int32 ask = testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
    CMyComPtr<ISequentialOutStream> output;
    RINOK(callback->GetStream(index, &output, ask))
    if (!testMode && !output && !Directory(entry)) {
        reader_->completed += entry.size;
        return S_OK;
    }
    RINOK(callback->PrepareOperation(ask))
    Int32 operation = NExtract::NOperationResult::kOK;
    if (!Directory(entry)) {
        if (!Regular(entry) && !Symlink(entry))
            operation = NExtract::NOperationResult::kUnsupportedMethod;
        else {
            UInt64 position = 0;
            std::array<Byte, 65536> buffer;
            while (position < entry.size) {
                size_t count = 0;
                const HRESULT result =
                    reader_->Read(entry.inode, position, buffer.data(), buffer.size(), &count);
                if (result == S_FALSE || result == E_NOTIMPL || (!count && result == S_OK)) {
                    operation = result == E_NOTIMPL ? NExtract::NOperationResult::kUnsupportedMethod
                                                    : NExtract::NOperationResult::kDataError;
                    break;
                }
                RINOK(result)
                if (output)
                    RINOK(WriteStream(output, buffer.data(), count))
                position += count;
                reader_->completed += count;
                RINOK(callback->SetCompleted(&reader_->completed))
            }
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
        if (!Directory(items_[index].entry)) {
            if (items_[index].entry.size > UINT64_MAX - total)
                return E_INVALIDARG;
            total += items_[index].entry.size;
        }
    }
    RINOK(callback->SetTotal(total))
    std::lock_guard<std::recursive_mutex> lock(reader_->mutex);
    reader_->completed = 0;
    reader_->extracting = callback;
    struct Reset {
        XfsArchiveReader &r;
        ~Reset() { r.extracting = nullptr; }
    } reset{*reader_};
    for (UInt32 i = 0; i < count; ++i)
        RINOK(ExtractItem(all ? i : indices[i], testMode, callback))
    return S_OK;
    COM_TRY_END
}
static const Byte kProps[] = {kpidPath,  kpidIsDir, kpidSize,   kpidPosixAttrib,
                              kpidINode, kpidMTime, kpidSymLink};
static const Byte kArcProps[] = {kpidPhySize, kpidClusterSize, kpidReadOnly};
IMP_IInArchive_Props IMP_IInArchive_ArcProps static const Byte signature[] = {'X', 'F', 'S', 'B'};
REGISTER_ARC_I("XFS", "xfs img", nullptr, 0xE9, signature, 0, NArcInfoFlags::kPureStartOpen,
               nullptr)
} // namespace NArchive::NXfs
