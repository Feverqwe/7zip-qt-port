#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <numeric>
#include <print>
#include <ranges>
#include <span>

import sevenzip.ai.foundation;

auto main() -> int {
    constexpr std::array samples{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F};
    const std::span<const float> data{samples};

    return sevenzip::ai::batch_size(2)
        .transform([data](unsigned size) {
            std::ranges::for_each(data | std::views::chunk(size), [](auto chunk) {
                const std::span<const float> batch{chunk};
                std::println("Batch sum: {}", std::accumulate(batch.begin(), batch.end(), 0.0F));
            });
            return EXIT_SUCCESS;
        })
        .or_else(
            [](sevenzip::ai::BatchError error) -> std::expected<int, sevenzip::ai::BatchError> {
                std::println(stderr, "Invalid batch size");
                return std::unexpected(error);
            })
        .value_or(EXIT_FAILURE);
}
