#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace vector_query {

struct TradeColumnView {
    std::span<const float> prices;
    std::span<const std::uint32_t> volumes;
    std::span<const std::uint32_t> symbol_ids;

    [[nodiscard]] std::size_t size() const noexcept { return prices.size(); }
    [[nodiscard]] bool has_consistent_sizes() const noexcept {
        return prices.size() == volumes.size() && prices.size() == symbol_ids.size();
    }
};

struct TradeColumns {
    std::vector<float> prices;
    std::vector<std::uint32_t> volumes;
    std::vector<std::uint32_t> symbol_ids;

    [[nodiscard]] std::size_t size() const noexcept { return prices.size(); }
    [[nodiscard]] bool has_consistent_sizes() const noexcept {
        return prices.size() == volumes.size() && prices.size() == symbol_ids.size();
    }
    // Spans borrow storage; reject temporary owners to prevent an immediate dangling view.
    [[nodiscard]] TradeColumnView view() const& noexcept { return {prices, volumes, symbol_ids}; }
    TradeColumnView view() const&& = delete;
};

} // namespace vector_query
