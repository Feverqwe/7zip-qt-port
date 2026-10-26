module;

#include <cstdint>
#include <expected>
#include <span>
#include <variant>

export module sevenzip.ai.foundation;

export namespace sevenzip::ai {

enum class BatchError : std::uint8_t { NonPositive };

template <typename... Cases> struct Overloaded : Cases... {
    using Cases::operator()...;
};

using SampleSource = std::variant<std::span<const float>, BatchError>;
using SampleResult = std::expected<std::span<const float>, BatchError>;

auto sample_data(const SampleSource &source) noexcept -> SampleResult {
    return std::visit(
        Overloaded{[](std::span<const float> data) -> SampleResult { return data; },
                   [](BatchError error) -> SampleResult { return std::unexpected(error); }},
        source);
}

auto batch_size(int requested) noexcept -> std::expected<unsigned, BatchError> {
    if (requested <= 0)
        return std::unexpected(BatchError::NonPositive);
    return static_cast<unsigned>(requested);
}

} // namespace sevenzip::ai
