#pragma once

#include "vector_query/data/trade_columns.hpp"
#include "vector_query/query/query.hpp"
#include "vector_query/query/result.hpp"

namespace vector_query {

// Validates equal column sizes (invalid_argument), then [begin, end) (out_of_range).
// Empty ranges are valid. The caller owns storage throughout the call.
[[nodiscard]] PartialResult execute_scalar(TradeColumnView columns, const Query& query,
                                           std::size_t begin, std::size_t end);

} // namespace vector_query
