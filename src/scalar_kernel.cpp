#include "vector_query/kernels/scalar_kernel.hpp"

#include <stdexcept>

namespace vector_query {

PartialResult execute_scalar(TradeColumnView columns, const Query& query, std::size_t begin,
                             std::size_t end) {
    if (!columns.has_consistent_sizes()) {
        throw std::invalid_argument("trade columns must have identical row counts");
    }
    if (begin > end || end > columns.size()) {
        throw std::out_of_range("query range must satisfy begin <= end <= size");
    }
    PartialResult result;
    for (std::size_t index = begin; index < end; ++index) {
        if (columns.volumes[index] > query.minimum_volume &&
            columns.symbol_ids[index] == query.target_symbol) {
            result.price_sum += static_cast<double>(columns.prices[index]);
            ++result.match_count;
        }
    }
    return result;
}

} // namespace vector_query
