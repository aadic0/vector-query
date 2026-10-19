#pragma once

#include "vector_query/data/trade_columns.hpp"

namespace vector_query {

struct GeneratorConfig {
    std::size_t row_count{};
    std::uint64_t seed{};
    std::uint32_t symbol_count{};
    float minimum_price{};
    float maximum_price{};
    std::uint32_t minimum_volume{};
    std::uint32_t maximum_volume{};
};

// Invalid configuration throws std::invalid_argument, including for zero rows.
// Repeatable within the same toolchain/standard-library and floating-point environment.
[[nodiscard]] TradeColumns generate_trades(const GeneratorConfig& config);

} // namespace vector_query
