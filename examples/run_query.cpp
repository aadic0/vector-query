#include "vector_query/data/data_generator.hpp"
#include "vector_query/kernels/scalar_kernel.hpp"

#include <exception>
#include <iomanip>
#include <iostream>

int main() {
    try {
        const auto columns =
            vector_query::generate_trades({100'000, 42, 16, 1.0F, 100.0F, 0, 1'000});
        const vector_query::Query query{500, 3};
        const auto result = vector_query::execute_scalar(columns.view(), query, 0, columns.size());
        std::cout << "Rows: " << columns.size() << "\nVolume > " << query.minimum_volume
                  << "; symbol_id = " << query.target_symbol << "\nMatches: " << result.match_count
                  << "\nPrice sum: " << std::fixed << std::setprecision(6) << result.price_sum
                  << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Query failed: " << error.what() << '\n';
        return 1;
    }
}
