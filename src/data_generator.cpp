#include "vector_query/data/data_generator.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

namespace vector_query {

TradeColumns generate_trades(const GeneratorConfig& config) {
    if (config.symbol_count == 0) {
        throw std::invalid_argument("symbol_count must be positive");
    }
    if (!std::isfinite(config.minimum_price) || !std::isfinite(config.maximum_price) ||
        config.minimum_price < 0.0F || config.minimum_price > config.maximum_price) {
        throw std::invalid_argument("price bounds must be finite, non-negative, and ordered");
    }
    if (config.minimum_volume > config.maximum_volume) {
        throw std::invalid_argument("volume bounds must be ordered");
    }

    TradeColumns columns;
    columns.prices.resize(config.row_count);
    columns.volumes.resize(config.row_count);
    columns.symbol_ids.resize(config.row_count);

    std::mt19937_64 random(config.seed);
    std::uniform_int_distribution<std::uint32_t> volumes(config.minimum_volume,
                                                         config.maximum_volume);
    std::uniform_int_distribution<std::uint32_t> symbols(0, config.symbol_count - 1);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const double minimum = static_cast<double>(config.minimum_price);
    const double width = static_cast<double>(config.maximum_price) - minimum;
    for (std::size_t index = 0; index < config.row_count; ++index) {
        // Double interpolation avoids overflow even with a maximum finite float bound.
        columns.prices[index] = static_cast<float>(minimum + width * unit(random));
        columns.volumes[index] = volumes(random);
        columns.symbol_ids[index] = symbols(random);
    }
    return columns;
}

} // namespace vector_query
