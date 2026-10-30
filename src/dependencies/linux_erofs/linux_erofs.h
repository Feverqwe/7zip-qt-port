// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include "formats/erofs/erofsreader.h"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>

namespace sevenzip::linux_erofs {
using Volume = std::unique_ptr<erofs_volume, decltype(&erofs_close_volume)>;
struct FileDeleter {
    void operator()(erofs_file *file) const noexcept { erofs_close_file(file); }
};
using File = std::unique_ptr<erofs_file, FileDeleter>;

// A mounted volume retains the source callbacks; their context must outlive it.
[[nodiscard]] inline auto open(const erofs_source &source) noexcept -> std::expected<Volume, int> {
    erofs_volume *opened = nullptr;
    const int error = erofs_open_volume(&source, &opened);
    Volume volume{opened, &erofs_close_volume};
    if (error)
        return std::unexpected(error);
    return volume;
}

// Files borrow their volume and must close before it.
[[nodiscard]] inline auto open_file(erofs_volume &volume, std::uint64_t nid) noexcept
    -> std::expected<File, int> {
    erofs_file *opened = nullptr;
    const int error = erofs_open_file(&volume, nid, &opened);
    File file{opened};
    if (error)
        return std::unexpected(error);
    return file;
}

struct ReadError {
    int code;
    std::size_t processed;
};

[[nodiscard]] inline auto read(erofs_file &file, std::uint64_t offset,
                               std::span<std::byte> data) noexcept
    -> std::expected<std::size_t, ReadError> {
    std::size_t processed = 0;
    const int error = erofs_pread_file(&file, offset, data.data(), data.size(), &processed);
    if (error)
        return std::unexpected(ReadError{error, processed});
    return processed;
}
} // namespace sevenzip::linux_erofs
