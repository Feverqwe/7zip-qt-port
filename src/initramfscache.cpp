// SPDX-License-Identifier: GPL-2.0-only
// Anonymous native scratch storage; no Qt/runtime-library dependency in 7z plugins.
#include "initramfscache.h"
#include "Common/MyCom.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {
class Cache final : public IInStream, public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_3(IInStream, ISequentialInStream, ISequentialOutStream)
#ifdef _WIN32
    HANDLE file_ = INVALID_HANDLE_VALUE;
#else
    FILE *file_ = nullptr;
#endif
    UInt64 position_ = 0, size_ = 0;

  public:
    ~Cache() {
#ifdef _WIN32
        if (file_ != INVALID_HANDLE_VALUE)
            CloseHandle(file_);
#else
        if (file_)
            fclose(file_);
#endif
    }
    HRESULT create() {
#ifdef _WIN32
        wchar_t folder[MAX_PATH + 1], path[MAX_PATH + 1];
        const DWORD count = GetTempPathW(MAX_PATH + 1, folder);
        if (!count || count > MAX_PATH || !GetTempFileNameW(folder, L"7zI", 0, path))
            return E_FAIL;
        file_ = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE |
                                FILE_FLAG_OPEN_REPARSE_POINT,
                            nullptr);
        if (file_ == INVALID_HANDLE_VALUE) {
            DeleteFileW(path);
            return E_FAIL;
        }
        BY_HANDLE_FILE_INFORMATION info;
        if (!GetFileInformationByHandle(file_, &info) ||
            (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) ||
            info.nNumberOfLinks != 1 || info.nFileSizeHigh || info.nFileSizeLow)
            return E_FAIL;
#else
        const char *directory = getenv("TMPDIR");
        std::string path = directory && *directory ? directory : "/tmp";
        path += "/7z-initramfs-XXXXXX";
        const int descriptor = mkstemp(path.data());
        if (descriptor < 0)
            return E_FAIL;
        if (unlink(path.c_str()) != 0) {
            close(descriptor);
            return E_FAIL;
        }
        if (fcntl(descriptor, F_SETFD, FD_CLOEXEC) == -1) {
            close(descriptor);
            return E_FAIL;
        }
        file_ = fdopen(descriptor, "w+b");
        if (!file_) {
            close(descriptor);
            return E_FAIL;
        }
#endif
        return S_OK;
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ >= size_ || !size)
            return S_OK;
        if (size > size_ - position_)
            size = UInt32(size_ - position_);
        UInt32 count;
#ifdef _WIN32
        DWORD actual = 0;
        const bool ok = ReadFile(file_, data, size, &actual, nullptr) != 0;
        count = actual;
#else
        count = UInt32(fread(data, 1, size, file_));
        const bool ok = !ferror(file_);
#endif
        position_ += count;
        if (processed)
            *processed = count;
        return ok ? S_OK : E_FAIL;
    }
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (size > UInt64(INT64_MAX) - position_)
            return E_INVALIDARG;
        UInt32 count;
#ifdef _WIN32
        DWORD actual = 0;
        const bool ok = WriteFile(file_, data, size, &actual, nullptr) != 0;
        count = actual;
#else
        count = UInt32(fwrite(data, 1, size, file_));
        const bool ok = !ferror(file_);
#endif
        position_ += count;
        if (position_ > size_)
            size_ = position_;
        if (processed)
            *processed = count;
        return ok && count == size ? S_OK : E_FAIL;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        UInt64 next;
        switch (origin) {
        case STREAM_SEEK_SET:
            next = 0;
            break;
        case STREAM_SEEK_CUR:
            next = position_;
            break;
        case STREAM_SEEK_END:
            next = size_;
            break;
        default:
            return STG_E_INVALIDFUNCTION;
        }
        if (offset < 0) {
            const UInt64 delta = UInt64(0) - UInt64(offset);
            if (delta > next)
                return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
            next -= delta;
        } else {
            if (next > UInt64(INT64_MAX) - UInt64(offset))
                return E_INVALIDARG;
            next += UInt64(offset);
        }
#ifdef _WIN32
        LARGE_INTEGER value;
        value.QuadPart = Int64(next);
        if (!SetFilePointerEx(file_, value, nullptr, FILE_BEGIN))
            return E_FAIL;
#else
        if (fseeko(file_, off_t(next), SEEK_SET) != 0)
            return E_FAIL;
#endif
        position_ = next;
        if (position)
            *position = next;
        return S_OK;
    }
};
} // namespace

HRESULT CreateInitramfsCache(IInStream **input, ISequentialOutStream **output) {
    *input = nullptr;
    *output = nullptr;
    Cache *cache = new Cache;
    CMyComPtr<IInStream> owner = cache;
    RINOK(cache->create())
    RINOK(owner.QueryInterface(IID_ISequentialOutStream, output))
    *input = owner.Detach();
    return S_OK;
}
