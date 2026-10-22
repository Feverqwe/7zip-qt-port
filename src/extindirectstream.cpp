// SPDX-License-Identifier: MIT
#include "extindirectstream.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/ComTry.h"
#include "Common/MyCom.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>

namespace {
class IndirectStream final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    std::shared_ptr<ext_volume> volume_;
    CMyComPtr<IInStream> source_;
    std::unique_ptr<ext_indirect, decltype(&ext_linux_indirect_close)> map_{
        nullptr, ext_linux_indirect_close};
    UInt32 mappedStart_ = 0, mappedCount_ = 0;
    unsigned blockBits_;
    UInt64 size_, position_ = 0, physical_ = 0;
    bool requireAllocated_;
    CExtCryptData crypt_;
    HRESULT inputResult_ = S_OK;

    static int ReadMetadata(void *opaque, uint64_t block, void *bytes, size_t size) noexcept {
        auto &self = *static_cast<IndirectStream *>(opaque);
        try {
            self.inputResult_ = InStream_SeekSet(self.source_, block << self.blockBits_);
            if (self.inputResult_ == S_OK)
                self.inputResult_ = ReadStream_FALSE(self.source_, bytes, size);
            return self.inputResult_ == S_OK ? 0 : -EIO;
        } catch (const std::bad_alloc &) {
            self.inputResult_ = E_OUTOFMEMORY;
            return -ENOMEM;
        } catch (...) {
            self.inputResult_ = E_FAIL;
            return -EIO;
        }
    }

    HRESULT Map(UInt32 logical) {
        if (mappedCount_ && logical >= mappedStart_ && logical - mappedStart_ < mappedCount_)
            return S_OK;
        const UInt64 total = (size_ + ((UInt64(1) << blockBits_) - 1)) >> blockBits_;
        const UInt32 count = UInt32(std::min<UInt64>(
            std::min<UInt64>(total - logical, (UInt64(1) << 32) - logical), INT_MAX));
        inputResult_ = S_OK;
        mappedCount_ = 0;
        const int result =
            ext_linux_indirect_lookup(map_.get(), logical, count, &physical_, &mappedCount_);
        RINOK(inputResult_)
        if (result) {
            mappedCount_ = 0;
            return result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
        }
        mappedStart_ = logical;
        return S_OK;
    }

  public:
    IndirectStream(const std::shared_ptr<ext_volume> &volume, IInStream *source, unsigned bits,
                   UInt64 size, bool requireAllocated, const CExtCryptInode &crypt)
        : volume_(volume), source_(source), blockBits_(bits), size_(size),
          requireAllocated_(requireAllocated) {
        if (crypt)
            crypt_.Init(crypt);
    }
    HRESULT Init(const Byte *refs, UInt32 inode) {
        const ext_indirect_services services{this, ReadMetadata};
        ext_indirect *map = nullptr;
        const int result = ext_linux_indirect_open(volume_.get(), inode, refs, &services, &map);
        map_.reset(map);
        return result == 0 ? S_OK : result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
    }
    HRESULT Scan(void *context, ExtIndirectRangeCallback visit) {
        const UInt64 total = (size_ + ((UInt64(1) << blockBits_) - 1)) >> blockBits_;
        for (UInt64 logical = 0; logical < total;) {
            if (logical > UINT32_MAX)
                return S_FALSE;
            RINOK(Map(UInt32(logical)))
            RINOK(visit(context, UInt32(logical), physical_, mappedCount_))
            logical += mappedCount_;
        }
        return S_OK;
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        COM_TRY_BEGIN
        if (processed)
            *processed = 0;
        if (!size || position_ >= size_)
            return S_OK;
        const UInt64 logical64 = position_ >> blockBits_;
        if (logical64 > UINT32_MAX)
            return S_FALSE;
        const UInt32 logical = UInt32(logical64);
        RINOK(Map(logical))
        const UInt64 offset = position_ & ((UInt64(1) << blockBits_) - 1);
        const UInt64 remaining =
            (UInt64(mappedCount_ - (logical - mappedStart_)) << blockBits_) - offset;
        size = UInt32(std::min<UInt64>(size, std::min<UInt64>(size_ - position_, remaining)));
        if (!physical_) {
            if (requireAllocated_)
                return S_FALSE;
            std::memset(data, 0, size);
            position_ += size;
            if (processed)
                *processed = size;
            return S_OK;
        }
        const UInt64 address = ((physical_ + logical - mappedStart_) << blockBits_) + offset;
        UInt32 done = 0;
        HRESULT result;
        if (crypt_.Defined())
            result = crypt_.Read(source_, position_, address, data, size, &done);
        else {
            RINOK(InStream_SeekSet(source_, address))
            result = source_->Read(data, size, &done);
        }
        position_ += done;
        if (processed)
            *processed = done;
        return result == S_OK && !done ? S_FALSE : result;
        COM_TRY_END
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const UInt64 base = origin == STREAM_SEEK_SET   ? 0
                            : origin == STREAM_SEEK_CUR ? position_
                                                        : size_;
        if (base > INT64_MAX || offset < -Int64(base) ||
            (offset > 0 && base > UInt64(INT64_MAX - offset)))
            return E_INVALIDARG;
        position_ = UInt64(Int64(base) + offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};
} // namespace

HRESULT CreateExtIndirectStream(const std::shared_ptr<ext_volume> &volume, IInStream *source,
                                const Byte *refs, UInt32 inode, unsigned bits, UInt64 size,
                                bool requireAllocated, const CExtCryptInode &crypt,
                                IInStream **result) {
    COM_TRY_BEGIN
    *result = nullptr;
    auto *spec = new IndirectStream(volume, source, bits, size, requireAllocated, crypt);
    CMyComPtr<IInStream> stream = spec;
    RINOK(spec->Init(refs, inode))
    *result = stream.Detach();
    return S_OK;
    COM_TRY_END
}

HRESULT CreateExtIndirectDirectoryStream(const std::shared_ptr<ext_volume> &volume,
                                         IInStream *source, const Byte *refs, UInt32 inode,
                                         unsigned bits, UInt64 size, void *context,
                                         ExtIndirectRangeCallback visit, IInStream **result) {
    COM_TRY_BEGIN
    *result = nullptr;
    auto *spec = new IndirectStream(volume, source, bits, size, false, {});
    CMyComPtr<IInStream> stream = spec;
    RINOK(spec->Init(refs, inode))
    const HRESULT status = spec->Scan(context, visit);
    if (status == S_OK || status == S_FALSE)
        *result = stream.Detach();
    return status;
    COM_TRY_END
}
