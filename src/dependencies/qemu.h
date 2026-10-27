// SPDX-License-Identifier: MIT
#pragma once

#include "../block/qcow.h"
#include "../block/qcow2.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

namespace sevenzip::qemu {
using SubclusterType = ::QCow2SubclusterType;
using enum ::QCow2SubclusterType;

inline constexpr std::uint64_t offset_mask = L2E_OFFSET_MASK;
inline constexpr std::uint64_t copied_flag = QCOW_OFLAG_COPIED;
inline constexpr std::uint64_t zero_flag = QCOW_OFLAG_ZERO;

[[nodiscard]] inline auto subcluster_type(bool extended, bool external, std::uint64_t entry,
                                          std::uint64_t bitmap, unsigned index) noexcept
    -> SubclusterType {
    return qcow2_classify_subcluster(extended, external, entry, bitmap, index);
}

struct CompressedCluster {
    std::uint64_t offset;
    std::size_t size;
};

[[nodiscard]] inline auto compressed_cluster(std::uint64_t entry, unsigned clusterBits) noexcept
    -> CompressedCluster {
    CompressedCluster cluster{};
    cluster.offset = qcow_get_compressed_cluster(entry, clusterBits, &cluster.size);
    return cluster;
}

class Extensions final {
    Qcow2HeaderExtensions state_{};

  public:
    Extensions() = default;
    Extensions(const Extensions &) = delete;
    auto operator=(const Extensions &) -> Extensions & = delete;
    ~Extensions() { reset(); }

    void reset() noexcept { qcow2_free_extensions(&state_); }

    // Views borrow this owner and remain valid until reset or destruction.
    [[nodiscard]] auto backing_format() const & noexcept -> std::string_view {
        return state_.image_backing_format ? std::string_view{state_.image_backing_format}
                                           : std::string_view{};
    }
    auto backing_format() const && -> std::string_view = delete;
    [[nodiscard]] auto data_file() const & noexcept -> std::string_view {
        return state_.image_data_file ? std::string_view{state_.image_data_file}
                                      : std::string_view{};
    }
    auto data_file() const && -> std::string_view = delete;
    [[nodiscard]] auto has_crypto() const noexcept -> bool { return state_.has_crypto; }

    template <typename Reader>
        requires requires(Reader &reader, std::uint64_t offset, std::span<std::byte> buffer) {
            { reader(offset, buffer) } noexcept -> std::convertible_to<std::int32_t>;
        }
    [[nodiscard]] auto read(Reader &reader, std::uint64_t start, std::uint64_t end,
                            std::uint64_t fileSize) noexcept -> std::expected<void, std::int32_t> {
        // The C parser invokes this bounded service synchronously.
        const auto callback = [](void *context, std::uint64_t offset, void *data,
                                 std::size_t size) noexcept -> std::int32_t {
            return (*static_cast<Reader *>(context))(offset,
                                                     {static_cast<std::byte *>(data), size});
        };
        const auto code = qcow2_read_extensions(callback, &reader, start, end, fileSize, &state_);
        if (code)
            return std::unexpected(code);
        return {};
    }
};
} // namespace sevenzip::qemu
