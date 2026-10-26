// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include "lz4frameio.h"
#include "stream_callbacks.h"

namespace sevenzip::lz4 {
enum class Status : std::uint8_t {
    ok = SEVENZIP_LZ4_OK,
    data_error = SEVENZIP_LZ4_DATA_ERROR,
    unexpected_end = SEVENZIP_LZ4_UNEXPECTED_END,
    memory_error = SEVENZIP_LZ4_MEMORY_ERROR,
    io_error = SEVENZIP_LZ4_IO_ERROR
};
struct Result {
    Status status;
    std::uint64_t consumed;
    std::uint64_t produced;
};
struct Source {
    std::uint64_t offset;
    std::uint64_t size;
};

template <dependencies::detail::SeekableIo Services>
[[nodiscard]] auto decode(Services &services, Source source,
                          std::uint64_t initialProduced = 0) noexcept -> Result {
    Result result{.status = Status::data_error, .consumed = 0, .produced = initialProduced};
    if (source.offset > source.size)
        return result;
    using Callbacks = dependencies::detail::StreamCallbacks<Services>;
    sevenzip_lz4_frame_io io{.opaque = &services,
                             .read = Callbacks::read,
                             .write = Callbacks::write,
                             .seek = Callbacks::seek,
                             .progress = Callbacks::checkpoint};
    result.status = static_cast<Status>(
        LZ4IO_decompressLZ4F(&io, source.offset, source.size, &result.consumed, &result.produced));
    return result;
}
} // namespace sevenzip::lz4
