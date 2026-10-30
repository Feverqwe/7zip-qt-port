// SPDX-License-Identifier: GPL-3.0-or-later
// Archive ABI adapter for the GRUB C reader. No update interface is exposed.
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/ComTry.h"
#include "Common/UTFConvert.h"
#include "Windows/PropVariant.h"
#include "formats/f2fs/f2fsarchivereader.h"
#include <array>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace NArchive::NF2fs {
namespace {
struct Item {
    UString path, symlink;
    f2fs_item info{};
};
bool IsDir(const Item &i) { return (i.info.mode & 0170000) == 0040000; }
bool HasData(const Item &i) {
    const unsigned type = i.info.mode & 0170000;
    return type == 0100000 || type == 0120000;
}
class FileStream final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    std::shared_ptr<F2fsArchiveReader> reader_;
    F2fsFile file_;
    UInt64 position_ = 0, size_;

  public:
    FileStream(std::shared_ptr<F2fsArchiveReader> reader, F2fsFile file, UInt64 size)
        : reader_(std::move(reader)), file_(std::move(file)), size_(size) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        COM_TRY_BEGIN
        if (processed)
            *processed = 0;
        std::lock_guard<std::recursive_mutex> lock(reader_->mutex);
        reader_->Reset();
        size_t done = 0;
        const int error = f2fs_pread(file_.get(), position_, data, size, &done);
        position_ += done;
        if (processed)
            *processed = UInt32(done);
        return reader_->Result(error);
        COM_TRY_END
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const UInt64 base = origin == STREAM_SEEK_SET   ? 0
                            : origin == STREAM_SEEK_CUR ? position_
                                                        : size_;
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
    std::shared_ptr<F2fsArchiveReader> reader_;
    std::vector<Item> items_;
    struct Directory {
        CHandler &self;
        UString prefix;
        std::vector<std::pair<UString, UInt32>> &pending;
        std::set<UInt32> &seen;
        std::set<std::string> names;
        HRESULT result = S_OK;
    };
    static int Emit(void *, const char *, uint32_t);
    HRESULT Add(Directory &, const char *, UInt32);
    HRESULT ReadTree(IArchiveOpenCallback *);
    HRESULT ExtractItem(UInt32, Int32, IArchiveExtractCallback *);
};

int CHandler::Emit(void *context, const char *name, uint32_t ino) {
    auto &dir = *static_cast<Directory *>(context);
    try {
        dir.result = dir.self.Add(dir, name, ino);
    } catch (const std::bad_alloc &) {
        dir.result = E_OUTOFMEMORY;
    } catch (...) {
        dir.result = E_FAIL;
    }
    return dir.result == S_OK ? 0 : 1;
}
HRESULT CHandler::Add(Directory &dir, const char *name, UInt32 ino) {
    if (!strcmp(name, ".") || !strcmp(name, ".."))
        return S_OK;
    if (!*name || strpbrk(name, "/\\:") || !dir.names.insert(name).second ||
        items_.size() >= UINT32_MAX)
        return S_FALSE;
    UString text;
    if (!ConvertUTF8ToUnicode(AString(name), text))
        return S_FALSE;
    Item item;
    item.path = dir.prefix + text;
    if (item.path.Len() > 32768)
        return S_FALSE;
    F2fsFile file;
    RINOK(reader_->OpenFile(ino, file))
    f2fs_file_info(file.get(), &item.info);
    if (IsDir(item)) {
        if (!dir.seen.insert(ino).second)
            return S_FALSE;
        if (!item.info.unsupported)
            dir.pending.emplace_back(item.path + L'/', ino);
    }
    if ((item.info.mode & 0170000) == 0120000 && !item.info.unsupported) {
        if (!item.info.size || item.info.size > 4096)
            return S_FALSE;
        std::string target(size_t(item.info.size), '\0');
        size_t done = 0;
        RINOK(reader_->Result(f2fs_pread(file.get(), 0, target.data(), target.size(), &done)))
        if (done != target.size() || target.find('\0') != std::string::npos ||
            !ConvertUTF8ToUnicode(AString(target.c_str()), item.symlink))
            return S_FALSE;
    }
    items_.push_back(std::move(item));
    reader_->files = items_.size();
    return S_OK;
}
HRESULT CHandler::ReadTree(IArchiveOpenCallback *callback) {
    std::vector<std::pair<UString, UInt32>> pending{{UString(), reader_->Root()}};
    std::set<UInt32> seen{reader_->Root()};
    reader_->opening = callback;
    struct ResetOpening {
        F2fsArchiveReader &reader;
        ~ResetOpening() { reader.opening = nullptr; }
    } reset{*reader_};
    while (!pending.empty()) {
        auto next = std::move(pending.back());
        pending.pop_back();
        F2fsFile file;
        RINOK(reader_->OpenFile(next.second, file))
        Directory directory{*this, std::move(next.first), pending, seen, {}, S_OK};
        const int error = f2fs_directory(file.get(), &directory, Emit);
        RINOK(directory.result)
        RINOK(reader_->Result(error))
    }
    return S_OK;
}
Z7_COM7F_IMF(CHandler::Open(IInStream *input, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN
    Close();
    reader_ = std::make_shared<F2fsArchiveReader>();
    HRESULT result = reader_->Open(input, callback);
    if (result == S_OK)
        result = ReadTree(callback);
    if (result != S_OK)
        Close();
    return result;
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
        prop = IsDir(item);
        break;
    case kpidSize:
        if (HasData(item))
            prop = item.info.size;
        break;
    case kpidPosixAttrib:
        prop = item.info.mode;
        break;
    case kpidUserId:
        prop = item.info.uid;
        break;
    case kpidGroupId:
        prop = item.info.gid;
        break;
    case kpidINode:
        prop = UInt64(item.info.ino);
        break;
    case kpidEncrypted:
        prop = bool(item.info.unsupported & 1);
        break;
    case kpidSymLink:
        if (!item.symlink.IsEmpty())
            prop = item.symlink;
        break;
    case kpidError:
        if (item.info.unsupported)
            prop = L"F2FS encrypted, compressed or verity data is unsupported";
        break;
    case kpidMTime:
    case kpidATime:
    case kpidCTime: {
        const UInt64 seconds = id == kpidMTime   ? item.info.mtime
                               : id == kpidATime ? item.info.atime
                                                 : item.info.ctime;
        if (seconds <= UINT64_MAX / 10000000 - 11644473600ULL) {
            const UInt64 ticks = (seconds + 11644473600ULL) * 10000000;
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
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidReadOnly)
        prop = true;
    if (id == kpidClusterSize)
        prop = UInt32(4096);
    prop.Detach(value);
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream)) {
    COM_TRY_BEGIN
    *stream = nullptr;
    if (!reader_ || index >= items_.size())
        return E_INVALIDARG;
    const auto &item = items_[index];
    if (!HasData(item))
        return S_FALSE;
    if (item.info.unsupported)
        return E_NOTIMPL;
    std::lock_guard<std::recursive_mutex> lock(reader_->mutex);
    reader_->Reset();
    F2fsFile file;
    RINOK(reader_->OpenFile(item.info.ino, file))
    *stream = new FileStream(reader_, std::move(file), item.info.size);
    (*stream)->AddRef();
    return S_OK;
    COM_TRY_END
}
HRESULT CHandler::ExtractItem(UInt32 index, Int32 testMode, IArchiveExtractCallback *callback) {
    const Item &item = items_[index];
    const Int32 ask = testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
    CMyComPtr<ISequentialOutStream> output;
    RINOK(callback->GetStream(index, &output, ask))
    if (!testMode && !output && !IsDir(item))
        return S_OK;
    RINOK(callback->PrepareOperation(ask))
    Int32 operation = NExtract::NOperationResult::kOK;
    if (item.info.unsupported)
        operation = NExtract::NOperationResult::kUnsupportedMethod;
    else if (!IsDir(item)) {
        if (!HasData(item))
            operation = NExtract::NOperationResult::kUnsupportedMethod;
        else {
            CMyComPtr<ISequentialInStream> input;
            const HRESULT opened = GetStream(index, &input);
            if (FAILED(opened))
                return opened;
            if (opened != S_OK) {
                output.Release();
                return callback->SetOperationResult(NExtract::NOperationResult::kDataError);
            }
            std::array<Byte, 65536> buffer;
            UInt64 position = 0;
            while (position < item.info.size) {
                RINOK(callback->SetCompleted(&reader_->completed))
                UInt32 done = 0;
                HRESULT result = input->Read(
                    buffer.data(),
                    UInt32(std::min<UInt64>(buffer.size(), item.info.size - position)), &done);
                if (FAILED(result))
                    return result;
                if (result != S_OK || !done) {
                    operation = NExtract::NOperationResult::kDataError;
                    break;
                }
                if (output)
                    RINOK(WriteStream(output, buffer.data(), done))
                position += done;
                reader_->completed += done;
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
        if (index >= items_.size() || total > UINT64_MAX - items_[index].info.size)
            return E_INVALIDARG;
        if (HasData(items_[index]))
            total += items_[index].info.size;
    }
    RINOK(callback->SetTotal(total))
    std::lock_guard<std::recursive_mutex> lock(reader_->mutex);
    reader_->completed = 0;
    reader_->extracting = callback;
    struct ResetExtracting {
        F2fsArchiveReader &reader;
        ~ResetExtracting() { reader.extracting = nullptr; }
    } reset{*reader_};
    for (UInt32 i = 0; i < count; ++i)
        RINOK(ExtractItem(all ? i : indices[i], testMode, callback))
    return callback->SetCompleted(&reader_->completed);
    COM_TRY_END
}
static const Byte kProps[] = {kpidPath,    kpidIsDir, kpidSize,    kpidPosixAttrib,
                              kpidMTime,   kpidATime, kpidCTime,   kpidUserId,
                              kpidGroupId, kpidINode, kpidSymLink, kpidError};
static const CStatProp kArcProps[] = {{nullptr, kpidReadOnly, VT_BOOL},
                                      {nullptr, kpidClusterSize, VT_UI4}};
IMP_IInArchive_Props IMP_IInArchive_ArcProps_WITH_NAME static const Byte signature[] = {0x10, 0x20,
                                                                                        0xf5, 0xf2};
REGISTER_ARC_I("F2FS", "f2fs img", nullptr, 0xF8, signature, 1024, 0, nullptr)
} // namespace NArchive::NF2fs
