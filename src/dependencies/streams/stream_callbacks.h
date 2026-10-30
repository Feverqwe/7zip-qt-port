// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sevenzip::dependencies::detail {
template <typename Services>
concept SequentialIo =
    requires(Services &io, std::span<std::byte> input, std::span<const std::byte> output,
             std::size_t &size, std::uint64_t position) {
        { io.read(input, size) } noexcept -> std::same_as<int>;
        { io.write(output) } noexcept -> std::same_as<int>;
        { io.checkpoint(position, position) } noexcept -> std::same_as<int>;
    };

template <typename Services>
concept SeekableIo = SequentialIo<Services> && requires(Services &io, std::uint64_t position) {
    { io.seek(position) } noexcept -> std::same_as<int>;
};

// The C loops invoke callbacks synchronously; Services outlives this call.
template <SequentialIo Services> struct StreamCallbacks {
    static auto read(void *opaque, void *data, std::size_t *size) noexcept -> int {
        const auto capacity = *size;
        const auto result = static_cast<Services *>(opaque)->read(
            {static_cast<std::byte *>(data), capacity}, *size);
        return result || *size > capacity;
    }
    static auto write(void *opaque, const void *data, std::size_t size) noexcept -> int {
        return static_cast<Services *>(opaque)->write({static_cast<const std::byte *>(data), size});
    }
    static auto seek(void *opaque, std::uint64_t position) noexcept -> int
        requires SeekableIo<Services>
    {
        return static_cast<Services *>(opaque)->seek(position);
    }
    static auto checkpoint(void *opaque, std::uint64_t consumed, std::uint64_t produced) noexcept
        -> int {
        return static_cast<Services *>(opaque)->checkpoint(consumed, produced);
    }
};
} // namespace sevenzip::dependencies::detail
