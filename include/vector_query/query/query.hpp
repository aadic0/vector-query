#pragma once
#include <cstdint>

namespace vector_query {
struct Query {
    std::uint32_t minimum_volume{};
    std::uint32_t target_symbol{};
};
} // namespace vector_query
