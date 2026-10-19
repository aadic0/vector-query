#pragma once
#include <cstdint>

namespace vector_query {
struct PartialResult {
    double price_sum{};
    std::uint64_t match_count{};
};
} // namespace vector_query
