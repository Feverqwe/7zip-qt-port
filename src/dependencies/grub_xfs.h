// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "xfsreader.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>

namespace sevenzip::grub_xfs {
using Volume = std::unique_ptr<xfs_volume, decltype(&xfs_close)>;
using Entry = xfs_entry;

template <typename Services>
concept Source = requires(Services &source, std::uint64_t offset, std::span<std::byte> data) {
    { source.read_source(offset, data) } noexcept -> std::same_as<int>;
    { source.checkpoint() } noexcept -> std::same_as<int>;
};

// The mounted volume borrows Services, which must outlive it.
template <Source Services>
[[nodiscard]] auto open(Services &services, std::uint64_t size) noexcept
    -> std::expected<Volume, int> {
    const xfs_source source{
        &services, size,
        [](void *opaque, std::uint64_t offset, void *data, std::size_t count) noexcept {
            return static_cast<Services *>(opaque)->read_source(
                offset, {static_cast<std::byte *>(data), count});
        },
        [](void *opaque) noexcept { return static_cast<Services *>(opaque)->checkpoint(); }};
    xfs_volume *opened = nullptr;
    const int error = xfs_open(&source, &opened);
    Volume volume{opened, &xfs_close};
    if (error)
        return std::unexpected(error);
    return volume;
}

struct ReadError {
    int code;
    std::size_t processed;
};

[[nodiscard]] inline auto read(xfs_volume &volume, std::uint64_t inode, std::uint64_t offset,
                               std::span<std::byte> data) noexcept
    -> std::expected<std::size_t, ReadError> {
    std::size_t processed = 0;
    const int error = xfs_read(&volume, inode, offset, data.data(), data.size(), &processed);
    if (error)
        return std::unexpected(ReadError{error, processed});
    return processed;
}

// Directory names and entries are borrowed only during this synchronous call.
template <typename Visitor>
    requires std::is_nothrow_invocable_r_v<int, Visitor &, std::string_view, const Entry &>
[[nodiscard]] auto list(xfs_volume &volume, std::uint64_t inode, Visitor &visitor) noexcept
    -> std::expected<void, int> {
    const int error = xfs_list(
        &volume, inode,
        [](void *opaque, const char *name, const xfs_entry *entry) noexcept {
            return std::invoke(*static_cast<Visitor *>(opaque), std::string_view{name}, *entry);
        },
        &visitor);
    if (error)
        return std::unexpected(error);
    return {};
}
} // namespace sevenzip::grub_xfs
