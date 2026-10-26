// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <brotli/decode.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace sevenzip::brotli {
enum class Status : std::uint8_t { needs_input, needs_output, done, data_error, memory_error };

class Decoder final {
  public:
    struct Step {
        Status status;
        std::size_t consumed;
        std::size_t produced;
    };

    Decoder() noexcept = default;
    Decoder(const Decoder &) = delete;
    auto operator=(const Decoder &) -> Decoder & = delete;
    Decoder(Decoder &&) = delete;
    auto operator=(Decoder &&) -> Decoder & = delete;

    [[nodiscard]] explicit operator bool() const noexcept { return state_ != nullptr; }

    [[nodiscard]] auto decode(std::span<const std::uint8_t> input,
                              std::span<std::uint8_t> output) noexcept -> Step {
        if (!state_)
            return {.status = Status::memory_error, .consumed = 0, .produced = 0};
        auto availableIn = input.size(), availableOut = output.size();
        auto *nextIn = input.data();
        auto *nextOut = output.data();
        const auto result = BrotliDecoderDecompressStream(state_.get(), &availableIn, &nextIn,
                                                          &availableOut, &nextOut, nullptr);
        return {.status = translate(result),
                .consumed = input.size() - availableIn,
                .produced = output.size() - availableOut};
    }

  private:
    [[nodiscard]] auto translate(BrotliDecoderResult result) const noexcept -> Status {
        switch (result) {
        case BROTLI_DECODER_RESULT_SUCCESS:
            return Status::done;
        case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT:
            return Status::needs_input;
        case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT:
            return Status::needs_output;
        default:
            const auto error = BrotliDecoderGetErrorCode(state_.get());
            return error <= BROTLI_DECODER_ERROR_ALLOC_CONTEXT_MODES &&
                           error >= BROTLI_DECODER_ERROR_ALLOC_BLOCK_TYPE_TREES
                       ? Status::memory_error
                       : Status::data_error;
        }
    }

    std::unique_ptr<BrotliDecoderState, decltype(&BrotliDecoderDestroyInstance)> state_{
        BrotliDecoderCreateInstance(nullptr, nullptr, nullptr), BrotliDecoderDestroyInstance};
};
} // namespace sevenzip::brotli
