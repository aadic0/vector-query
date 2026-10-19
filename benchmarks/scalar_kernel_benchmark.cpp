#include "vector_query/data/data_generator.hpp"
#include "vector_query/kernels/scalar_kernel.hpp"

#include <benchmark/benchmark.h>

#include <cstdint>

namespace {

void scalar_query(benchmark::State& state) {
    const auto rows = static_cast<std::size_t>(state.range(0));
    // A single symbol isolates the volume predicate: approximately 10% vs 90% matches.
    const auto columns = vector_query::generate_trades({rows, 42, 1, 1.0F, 100.0F, 1, 1'000});
    vector_query::Query query{static_cast<std::uint32_t>(state.range(1)), 0};
    const auto reference = vector_query::execute_scalar(columns.view(), query, 0, rows);
    auto view = columns.view();
    // Google Benchmark starts timing on entry to the range loop, after all setup above.
    for (auto iteration : state) {
        (void)iteration;
        benchmark::DoNotOptimize(view);
        benchmark::DoNotOptimize(query);
        auto result = vector_query::execute_scalar(view, query, 0, rows);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(rows));
    state.counters["match_fraction"] =
        static_cast<double>(reference.match_count) / static_cast<double>(rows);
    state.SetLabel("single symbol; rows are items; 12 logical bytes/row");
}

BENCHMARK(scalar_query)
    ->ArgsProduct({{1'024, 262'144, 8'388'608}, {100, 900}})
    ->ArgNames({"rows", "minimum_volume"})
    ->Unit(benchmark::kMicrosecond);

} // namespace
