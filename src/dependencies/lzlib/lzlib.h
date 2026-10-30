// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include "dependencies/streams/stream_callbacks.h"
#include "formats/lzip/lzipio.h"

namespace sevenzip::lzlib {
enum class Status : std::uint8_t {
    ok = SEVENZIP_LZIP_OK,
    data_error = SEVENZIP_LZIP_DATA_ERROR,
    unexpected_end = SEVENZIP_LZIP_UNEXPECTED_END,
    memory_error = SEVENZIP_LZIP_MEMORY_ERROR,
    trailing_data = SEVENZIP_LZIP_TRAILING_DATA,
    io_error = SEVENZIP_LZIP_IO_ERROR
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

template <dependencies::detail::SequentialIo Services>
[[nodiscard]] auto decode(Services &services, Source source,
                          std::uint64_t initialProduced = 0) noexcept -> Result {
    Result result{.status = Status::data_error, .consumed = 0, .produced = initialProduced};
    if (source.offset > source.size)
        return result;
    using Callbacks = dependencies::detail::StreamCallbacks<Services>;
    sevenzip_lzip_io io{.opaque = &services,
                        .read = Callbacks::read,
                        .write = Callbacks::write,
                        .progress = Callbacks::checkpoint};
    result.status = static_cast<Status>(SevenzipLzipDecompress(&io, source.offset, source.size,
                                                               &result.consumed, &result.produced));
    return result;
}
} // namespace sevenzip::lzlib
