// SPDX-License-Identifier: GPL-2.0-or-later
// 7-Zip IInArchive presentation/stream adapter for Linux fs/exfat.
#include "7zip/Archive/IArchive.h"
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/ComTry.h"
#include "Common/MyCom.h"
#include "Common/UTFConvert.h"
#include "Windows/PropVariant.h"
#include "Windows/TimeUtils.h"
#include "exfatreader.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <memory>
#include <set>
#include <vector>

namespace NArchive::NExfat {
namespace {
struct Item {
    UString path;
    exfat_item entry;
    bool incomplete = false;
};
struct Pending {
    UString path;
    exfat_cursor cursor;
    size_t index = size_t(-1);
    std::vector<uint32_t> ancestors;
};
bool ItemName(const exfat_item &item, UString &name) {
    UString utf16;
    for (unsigned i = 0; i < 255 && item.name[i]; ++i) {
        const auto ch = item.name[i];
        if (ch < 32 || ch == '/' || ch == '\\' || ch == ':')
            return false;
        utf16 += wchar_t(ch);
    }
    if (utf16.IsEmpty() || utf16 == L"." || utf16 == L"..")
        return false;
    // 7-Zip keeps UTF-16 code units in UString on every platform.
    name = utf16;
    return true;
}
bool EntryTime(UInt16 date, UInt16 time, Byte cs, Byte tz, FILETIME &result) {
    const exfat_timestamp value = exfat_entry_time(date, time, cs, tz);
    UInt64 ticks;
    if (!NWindows::NTime::UnixTime64_To_FileTime64(value.seconds, ticks))
        return false;
    ticks += value.nanoseconds / 100;
    result.dwLowDateTime = UInt32(ticks);
    result.dwHighDateTime = UInt32(ticks >> 32);
    return true;
}
} // namespace

class CHandler final : public IInArchive, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IInArchive)
    Z7_IFACE_COM7_IMP(IInArchive)
    CMyComPtr<IInStream> stream_;
    std::unique_ptr<exfat_volume, decltype(&exfat_close_volume)> volume_{nullptr,
                                                                         exfat_close_volume};
    std::vector<Item> items_;
    IArchiveOpenCallback *openCallback_ = nullptr;
    IArchiveExtractCallback *extractCallback_ = nullptr;
    HRESULT transportResult_ = S_OK;
    UInt64 completed_ = 0;
    bool directoryLimitations_ = false;
    bool rootIncomplete_ = false;
    static int Read(void *, uint64_t, void *, size_t);
    static int Poll(void *);
    HRESULT ReadTree();
    void MarkDirectoryIncomplete(size_t index) {
        directoryLimitations_ = true;
        if (index == size_t(-1))
            rootIncomplete_ = true;
        else
            items_[index].incomplete = true;
    }
    HRESULT ExtractItem(UInt32, Int32, IArchiveExtractCallback *);
};

enum { kpidReadLimitations = kpidUserDefined };

int CHandler::Poll(void *context) {
    auto &self = *static_cast<CHandler *>(context);
    if (self.transportResult_ != S_OK)
        return -EIO;
    if (self.openCallback_) {
        const UInt64 files = self.items_.size();
        self.transportResult_ = self.openCallback_->SetCompleted(&files, nullptr);
    } else if (self.extractCallback_)
        self.transportResult_ = self.extractCallback_->SetCompleted(&self.completed_);
    return self.transportResult_ == S_OK ? 0 : -EIO;
}

int CHandler::Read(void *context, uint64_t offset, void *buffer, size_t size) {
    auto &self = *static_cast<CHandler *>(context);
    self.transportResult_ = InStream_SeekSet(self.stream_, offset);
    if (self.transportResult_ == S_OK)
        self.transportResult_ = ReadStream_FALSE(self.stream_, buffer, size);
    return self.transportResult_ == S_OK ? 0 : -EIO;
}

HRESULT CHandler::ReadTree() {
    Pending root;
    exfat_root_cursor(volume_.get(), &root.cursor);
    root.ancestors.push_back(root.cursor.cluster);
    std::vector<Pending> pending{root};
    while (!pending.empty()) {
        Pending directory = pending.back();
        pending.pop_back();
        for (;;) {
            Item item;
            const auto position = directory.cursor.position;
            const int result = exfat_next_item(volume_.get(), &directory.cursor, &item.entry);
            if (directory.cursor.incomplete)
                MarkDirectoryIncomplete(directory.index);
            if (result < 0) {
                if (transportResult_ == E_ABORT || transportResult_ == E_OUTOFMEMORY)
                    return transportResult_;
                if (result == -ENOMEM)
                    return E_OUTOFMEMORY;
                MarkDirectoryIncomplete(directory.index);
                transportResult_ = S_OK;
                if (directory.cursor.position > position)
                    continue; // Linux established the next record boundary.
                break;
            }
            if (!result)
                break;
            UString name;
            if (!ItemName(item.entry, name)) {
                MarkDirectoryIncomplete(directory.index);
                continue;
            }
            item.path = directory.path;
            item.path += name;
            if ((item.entry.attributes & 0x10) && item.entry.size) {
                if (directory.ancestors.size() > 256 ||
                    std::find(directory.ancestors.begin(), directory.ancestors.end(),
                              item.entry.cluster) != directory.ancestors.end()) {
                    item.incomplete = true;
                    directoryLimitations_ = true;
                } else {
                    Pending child;
                    child.path = item.path;
                    child.path += L'/';
                    child.index = items_.size();
                    child.ancestors = directory.ancestors;
                    child.ancestors.push_back(item.entry.cluster);
                    exfat_item_cursor(&item.entry, &child.cursor);
                    pending.push_back(std::move(child));
                }
            }
            items_.push_back(item);
            if (Poll(this))
                return transportResult_;
        }
    }
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN
    Close();
    stream_ = stream;
    openCallback_ = callback;
    struct Cleanup {
        CHandler &handler;
        bool accepted = false;
        ~Cleanup() {
            handler.openCallback_ = nullptr;
            if (!accepted)
                handler.Close();
        }
    } cleanup{*this};
    UInt64 size;
    const HRESULT sized = InStream_GetSize_SeekToEnd(stream, size);
    if (sized != S_OK) {
        Close();
        return sized;
    }
    exfat_source source{this, size, Read, Poll};
    exfat_volume *volume = nullptr;
    const int result = exfat_open_volume(&source, &volume);
    volume_.reset(volume);
    HRESULT hr = result ? (transportResult_ == S_OK ? S_FALSE : transportResult_) : ReadTree();
    openCallback_ = nullptr;
    if (hr != S_OK)
        Close();
    cleanup.accepted = hr == S_OK;
    return hr;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close()) {
    volume_.reset();
    stream_.Release();
    items_.clear();
    openCallback_ = nullptr;
    extractCallback_ = nullptr;
    transportResult_ = S_OK;
    completed_ = 0;
    directoryLimitations_ = false;
    rootIncomplete_ = false;
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
    FILETIME time;
    const bool directory = (item.entry.attributes & 0x10) != 0;
    switch (id) {
    case kpidError:
        if (item.incomplete)
            prop = "The exFAT directory could not be read completely.";
        break;
    case kpidPath:
        prop = item.path;
        break;
    case kpidIsDir:
        prop = directory;
        break;
    case kpidSize:
        if (!directory)
            prop = UInt64(item.entry.size);
        break;
    case kpidPackSize:
        if (!directory)
            prop = UInt64(item.entry.size);
        break;
    case kpidAttrib:
        prop = UInt32(item.entry.attributes);
        break;
    case kpidCTime:
        if (EntryTime(item.entry.create_date, item.entry.create_time, item.entry.create_cs,
                      item.entry.create_tz, time))
            prop = time;
        break;
    case kpidMTime:
        if (EntryTime(item.entry.modify_date, item.entry.modify_time, item.entry.modify_cs,
                      item.entry.modify_tz, time))
            prop = time;
        break;
    case kpidATime:
        if (EntryTime(item.entry.access_date, item.entry.access_time, 0, item.entry.access_tz,
                      time))
            prop = time;
        break;
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID id, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidErrorFlags && rootIncomplete_)
        prop = UInt32(kpv_ErrorFlags_HeadersError);
    else if (id == kpidReadOnly && volume_)
        prop = true;
    else if (id == kpidPhySize && volume_)
        prop = UInt64(exfat_volume_size(volume_.get()));
    else if (id == kpidFreeSpace && volume_)
        prop = UInt64(exfat_volume_free_size(volume_.get()));
    else if (id == kpidClusterSize && volume_)
        prop = UInt32(exfat_volume_cluster_size(volume_.get()));
    else if (id == kpidSectorSize && volume_)
        prop = UInt32(exfat_volume_sector_size(volume_.get()));
    else if (id == kpidReadLimitations && volume_) {
        const UInt16 status = exfat_volume_status(volume_.get());
        AString warning;
        if (status & 2)
            warning = "Volume was not properly unmounted. Some data may be corrupt.";
        if (status & 4) {
            if (!warning.IsEmpty())
                warning += '\n';
            warning += "Medium has reported failures. Some data may be lost.";
        }
        if (directoryLimitations_) {
            if (!warning.IsEmpty())
                warning += '\n';
            warning += "Some exFAT directories could not be read completely. Files or names may be "
                       "missing. Readable files remain accessible.";
        }
        if (!warning.IsEmpty())
            prop = warning;
    } else if (id == kpidVolumeName && volume_) {
        UString label;
        for (const uint16_t *p = exfat_volume_label(volume_.get()); *p; ++p)
            label += wchar_t(*p);
        prop = label;
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

HRESULT CHandler::ExtractItem(UInt32 index, Int32 testMode, IArchiveExtractCallback *callback) {
    const auto &item = items_[index];
    const bool directory = (item.entry.attributes & 0x10) != 0;
    const Int32 askMode = testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
    CMyComPtr<ISequentialOutStream> output;
    RINOK(callback->GetStream(index, &output, askMode))
    if (!testMode && !output && !directory) {
        completed_ += item.entry.size;
        return S_OK;
    }
    RINOK(callback->PrepareOperation(askMode))
    Int32 operation =
        item.incomplete ? NExtract::NOperationResult::kDataError : NExtract::NOperationResult::kOK;
    if (!directory) {
        exfat_cursor cursor;
        exfat_item_cursor(&item.entry, &cursor);
        std::array<Byte, 65536> buffer;
        while (cursor.position < item.entry.size) {
            size_t processed = 0;
            if (exfat_read_item(volume_.get(), &item.entry, &cursor, buffer.data(), buffer.size(),
                                &processed) ||
                !processed) {
                if (transportResult_ != S_OK)
                    return transportResult_;
                operation = NExtract::NOperationResult::kDataError;
                break;
            }
            if (output)
                RINOK(WriteStream(output, buffer.data(), processed))
            completed_ += processed;
            if (Poll(this))
                return transportResult_;
        }
    }
    output.Release();
    return callback->SetOperationResult(operation);
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 count, Int32 testMode,
                               IArchiveExtractCallback *callback)) {
    COM_TRY_BEGIN
    if (!volume_)
        return E_FAIL;
    const bool all = count == UInt32(-1);
    if (all)
        count = UInt32(items_.size());
    UInt64 total = 0;
    for (UInt32 i = 0; i < count; ++i) {
        const UInt32 index = all ? i : indices[i];
        if (index >= items_.size())
            return E_INVALIDARG;
        if (!(items_[index].entry.attributes & 0x10)) {
            if (UINT64_MAX - total < items_[index].entry.size)
                return E_INVALIDARG;
            total += items_[index].entry.size;
        }
    }
    RINOK(callback->SetTotal(total))
    completed_ = 0;
    transportResult_ = S_OK;
    extractCallback_ = callback;
    struct ResetCallback {
        IArchiveExtractCallback *&callback;
        ~ResetCallback() { callback = nullptr; }
    } reset{extractCallback_};
    HRESULT result = S_OK;
    for (UInt32 i = 0; i < count && result == S_OK; ++i)
        result = ExtractItem(all ? i : indices[i], testMode, callback);
    extractCallback_ = nullptr;
    if (result == S_OK && rootIncomplete_ &&
        (all || (count && count == items_.size() &&
                 std::set<UInt32>(indices, indices + count).size() == count))) {
        CMyComPtr<IArchiveExtractCallbackMessage2> messages;
        callback->QueryInterface(IID_IArchiveExtractCallbackMessage2, (void **)&messages);
        if (messages)
            return messages->ReportExtractResult(NEventIndexType::kNoIndex, 0,
                                                 NExtract::NOperationResult::kHeadersError);
        return S_FALSE;
    }
    return result;
    COM_TRY_END
}

static const Byte kProps[] = {kpidPath,  kpidIsDir, kpidSize,  kpidPackSize, kpidAttrib,
                              kpidCTime, kpidMTime, kpidATime, kpidError};
static const CStatProp kArcProps[] = {
    {nullptr, kpidReadOnly, VT_BOOL},  {nullptr, kpidErrorFlags, VT_UI4},
    {nullptr, kpidPhySize, VT_UI8},    {nullptr, kpidVolumeName, VT_BSTR},
    {nullptr, kpidFreeSpace, VT_UI8},  {nullptr, kpidClusterSize, VT_UI4},
    {nullptr, kpidSectorSize, VT_UI4}, {"Read-only limitations", kpidReadLimitations, VT_BSTR}};
IMP_IInArchive_Props IMP_IInArchive_ArcProps_WITH_NAME static const Byte signature[] = {
    'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '};
REGISTER_ARC_I("exFAT", "exfat img", nullptr, 0xF0, signature, 3, 0, nullptr)
} // namespace NArchive::NExfat
