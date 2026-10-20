// SPDX-License-Identifier: GPL-2.0-only
// 7-Zip stream/ownership adapter for NTFS3 ni_read_frame and WOF C readers.
#include "ntfswofstream.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/MyBuffer2.h"
#include "Common/MyCom.h"
#include "ntfsdecoder.h"
#include <cerrno>
#include <cstring>
#include <new>

namespace {
class WofStream final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    CMyComPtr<IInStream> ads_;
    sevenzip_ntfs_wof *reader_ = nullptr;
    CAlignedBuffer frame_;
    UInt64 size_, valid_, position_ = 0, cached_ = UINT64_MAX;
    unsigned bits_;
    HRESULT failure_ = S_OK;
    static int readSource(void *opaque, uint64_t offset, void *data, size_t size) {
        auto &self = *static_cast<WofStream *>(opaque);
        if (!self.ads_) {
            self.failure_ = S_FALSE;
            return -1;
        }
        try {
            self.failure_ = InStream_SeekSet(self.ads_, offset);
            if (self.failure_ == S_OK)
                self.failure_ = ReadStream_FALSE(self.ads_, data, size);
        } catch (const std::bad_alloc &) {
            self.failure_ = E_OUTOFMEMORY;
        } catch (...) {
            self.failure_ = E_FAIL;
        }
        return self.failure_ == S_OK ? 0 : -1;
    }

  public:
    WofStream(IInStream *ads, UInt64 size, UInt64 valid, unsigned bits)
        : ads_(ads), size_(size), valid_(valid), bits_(bits) {}
    ~WofStream() { sevenzip_ntfs_wof_destroy(reader_); }
    HRESULT init(UInt64 adsSize) {
        frame_.Alloc(size_t(1) << bits_);
        if (!frame_.IsAllocated())
            return E_OUTOFMEMORY;
        if (size_) {
            reader_ = sevenzip_ntfs_wof_create(size_, adsSize, bits_, readSource, this);
            if (!reader_)
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (!size || position_ >= size_)
            return S_OK;
        UInt64 remaining = size_ - position_;
        if (size > remaining)
            size = UInt32(remaining);
        if (position_ >= valid_) {
            std::memset(data, 0, size);
        } else {
            remaining = valid_ - position_;
            if (size > remaining)
                size = UInt32(remaining);
            const UInt64 frame = position_ >> bits_;
            if (cached_ != frame) {
                failure_ = S_OK;
                cached_ = UINT64_MAX;
                const int result = sevenzip_ntfs_wof_read_frame(reader_, frame, frame_);
                if (result == -EOPNOTSUPP)
                    return E_NOTIMPL;
                if (result)
                    return failure_ == S_OK ? (result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE)
                                            : failure_;
                cached_ = frame;
            }
            const size_t offset = size_t(position_ & ((UInt64(1) << bits_) - 1));
            const size_t available = (size_t(1) << bits_) - offset;
            if (size > available)
                size = UInt32(available);
            std::memcpy(data, frame_ + offset, size);
        }
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 distance, UInt32 origin, UInt64 *position)) override {
        UInt64 base;
        switch (origin) {
        case STREAM_SEEK_SET:
            base = 0;
            break;
        case STREAM_SEEK_CUR:
            base = position_;
            break;
        case STREAM_SEEK_END:
            base = size_;
            break;
        default:
            return STG_E_INVALIDFUNCTION;
        }
        if (distance < 0) {
            const UInt64 delta = UInt64(0) - UInt64(distance);
            if (delta > base)
                return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
            base -= delta;
        } else {
            if (base > UInt64(INT64_MAX) - UInt64(distance))
                return E_INVALIDARG;
            base += UInt64(distance);
        }
        position_ = base;
        if (position)
            *position = base;
        return S_OK;
    }
};
} // namespace

HRESULT SevenzipNtfsWofStream(IInStream *ads, UInt64 size, UInt64 valid, UInt64 adsSize,
                              unsigned bits, IInStream **stream) {
    auto *object = new WofStream(ads, size, valid, bits);
    CMyComPtr<IInStream> owner = object;
    const HRESULT result = object->init(adsSize);
    if (result != S_OK)
        return result;
    *stream = owner.Detach();
    return S_OK;
}
