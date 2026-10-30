// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <zlib.h>

namespace sevenzip::zlib {
enum class Status { ok, stream_end, memory_error, data_error };

class Inflater final {
  public:
    struct Step {
        Status status;
        std::size_t consumed;
        std::size_t produced;
    };

    Inflater() noexcept : status_(translate(inflateInit2(&state_, -MAX_WBITS))) {}
    ~Inflater() {
        if (state_.state)
            inflateEnd(&state_);
    }
    Inflater(const Inflater &) = delete;
    Inflater &operator=(const Inflater &) = delete;
    Inflater(Inflater &&) = delete;
    Inflater &operator=(Inflater &&) = delete;

    [[nodiscard]] Status status() const noexcept { return status_; }

    [[nodiscard]] Step decode(std::span<const unsigned char> input,
                              std::span<unsigned char> output) noexcept {
        if (status_ != Status::ok)
            return {status_, 0, 0};
        // zlib's streaming counters are uInt even on platforms with 64-bit size_t.
        const auto inputSize = static_cast<uInt>((std::min)(input.size(), max_chunk));
        const auto outputSize = static_cast<uInt>((std::min)(output.size(), max_chunk));
        state_.next_in = const_cast<Bytef *>(input.data());
        state_.avail_in = inputSize;
        state_.next_out = output.data();
        state_.avail_out = outputSize;
        const auto result = translate(inflate(&state_, Z_NO_FLUSH));
        return {result, inputSize - state_.avail_in, outputSize - state_.avail_out};
    }

  private:
    static constexpr std::size_t max_chunk = (std::numeric_limits<uInt>::max)();
    static Status translate(int result) noexcept {
        switch (result) {
        case Z_OK:
            return Status::ok;
        case Z_STREAM_END:
            return Status::stream_end;
        case Z_MEM_ERROR:
            return Status::memory_error;
        default:
            return Status::data_error;
        }
    }

    z_stream state_{};
    Status status_;
};

[[nodiscard]] inline std::uint32_t initial_checksum() noexcept { return adler32(0, Z_NULL, 0); }

[[nodiscard]] inline std::uint32_t update_checksum(std::uint32_t seed,
                                                   std::span<const unsigned char> data) noexcept {
    return data.empty() ? seed
                        : static_cast<std::uint32_t>(adler32_z(seed, data.data(), data.size()));
}
} // namespace sevenzip::zlib
