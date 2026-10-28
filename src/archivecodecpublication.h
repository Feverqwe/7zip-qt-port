#pragma once

#include <atomic>
#include <memory>
#include <utility>
#include <version>

class ArchiveCodecCatalog;

// All access to a published immutable catalog retains its native provider lease.
class ArchiveCodecPublication final {
    using Catalog = std::shared_ptr<const ArchiveCodecCatalog>;
#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    std::atomic<Catalog> value_;
#else
    // Current libc++ lacks P0718R2. Keep its atomic-access API encapsulated.
    Catalog value_;
#endif

  public:
    ArchiveCodecPublication() = default;
    ArchiveCodecPublication(const ArchiveCodecPublication &) = delete;
    ArchiveCodecPublication &operator=(const ArchiveCodecPublication &) = delete;
    [[nodiscard]] Catalog load() const {
#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
        return value_.load();
#else
        return std::atomic_load(&value_);
#endif
    }
    void store(Catalog catalog) {
#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
        value_.store(std::move(catalog));
#else
        std::atomic_store(&value_, std::move(catalog));
#endif
    }
};
