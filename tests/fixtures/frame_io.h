// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

struct OversizedFrameRead {
    bool wrote = false;
    auto read(std::span<std::byte> buffer, std::size_t &size) noexcept -> int {
        std::ranges::fill(buffer, std::byte{});
        size = buffer.size() + 1;
        return 0;
    }
    auto write(std::span<const std::byte>) noexcept -> int {
        wrote = true;
        return 0;
    }
    auto seek(std::uint64_t) noexcept -> int { return 0; }
    auto checkpoint(std::uint64_t, std::uint64_t) noexcept -> int { return 0; }
};
