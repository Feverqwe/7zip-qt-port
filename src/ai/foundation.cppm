module;

#include <cstdint>
#include <expected>

export module sevenzip.ai.foundation;

export namespace sevenzip::ai {

enum class BatchError : std::uint8_t { NonPositive };

auto batch_size(int requested) noexcept -> std::expected<unsigned, BatchError> {
    if (requested <= 0)
        return std::unexpected(BatchError::NonPositive);
    return static_cast<unsigned>(requested);
}

} // namespace sevenzip::ai
