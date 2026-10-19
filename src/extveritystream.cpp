// SPDX-License-Identifier: MIT
#include "extveritystream.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Common/StreamUtils.h"
#include "extverityreader.h"
#include <cerrno>
#include <cstring>

namespace {
// The existing 7-Zip cache owns seek/read behavior; only block acquisition changes.
class CExtVerityStream final : public CCachedInStream {
    CMyComPtr<IInStream> raw_;
    ext_verity_reader *verity_ = nullptr;
    UInt64 fileSize_ = 0;
    unsigned blockBits_ = 0;
    HRESULT inputError_ = S_OK;

    static int ReadRaw(void *context, uint64_t position, void *data, size_t size) {
        auto &self = *static_cast<CExtVerityStream *>(context);
        HRESULT result = InStream_SeekSet(self.raw_, position);
        if (result == S_OK)
            result = ReadStream_FALSE(self.raw_, data, size);
        if (result != S_OK)
            self.inputError_ = result;
        return result == S_OK ? 0 : -EIO;
    }

    HRESULT ReadBlock(UInt64 index, Byte *data, size_t amount) override {
        inputError_ = S_OK;
        const UInt64 position = index << blockBits_;
        const size_t blockSize = size_t(1) << blockBits_;
        // Like Linux's page cache, hide physical bytes past logical EOF before hashing.
        memset(data, 0, blockSize);
        if (ReadRaw(this, position, data, amount) != 0)
            return inputError_;
        const int status = ext_verity_verify(verity_, position, data);
        if (inputError_ != S_OK)
            return inputError_;
        return status == 0 ? S_OK : E_FAIL;
    }

  public:
    ~CExtVerityStream() override { ext_verity_close(verity_); }

    HRESULT Open(IInStream *raw, UInt64 fileSize, unsigned blockBits, UInt32 allocatedEnd) {
        raw_ = raw;
        fileSize_ = fileSize;
        const int status = ext_verity_open(ReadRaw, this, fileSize, blockBits, allocatedEnd,
                                           &verity_, &blockBits_);
        if (inputError_ != S_OK)
            return inputError_;
        if (status != 0)
            return status == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
        if (!Alloc(blockBits_, 2))
            return E_OUTOFMEMORY;
        Init(fileSize_);
        return S_OK;
    }
};
} // namespace

HRESULT CreateExtVerityStream(IInStream *raw, UInt64 fileSize, unsigned blockBits,
                              UInt32 allocatedEnd, IInStream **result) {
    *result = nullptr;
    auto *stream = new CExtVerityStream;
    CMyComPtr<IInStream> owner = stream;
    const HRESULT status = stream->Open(raw, fileSize, blockBits, allocatedEnd);
    if (status == S_OK)
        *result = owner.Detach();
    return status;
}
