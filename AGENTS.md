# VectorQuery contributor instructions

VectorQuery is a columnar analytical executor for controlled C++ performance
experiments. Keep implementations small enough to explain and benchmark.

- Configure/build/test: `cmake --preset debug`, `cmake --build --preset debug --parallel 2`,
  `ctest --preset debug`. Repeat with `release` and `sanitizers` presets as relevant.
- Demo: `./build/release/vector_query_demo`. Benchmark:
  `./build/release/vector_query_benchmarks --benchmark_min_time=0.2s --benchmark_repetitions=3`.
- Use C++23, `vector_query`, PascalCase types, snake_case functions/variables,
  fixed-width integers where appropriate, spans, RAII, and `#pragma once`.
  Format project C++ with `.clang-format`; keep warnings target-scoped.
- Correctness precedes optimization. Compare optimized kernels with the scalar
  reference: exact counts, justified floating-point tolerance, arbitrary valid ranges.
- Keep data generation, allocation, and file loading outside query timings.
- Never apply global ISA-specific compiler flags such as AVX2 or `-march=native`.
- Run relevant builds/tests and report observed results and any blocked checks.
- Follow `docs/roadmap.md`; do not add lock-free, NUMA, or adaptive behavior early.
- Do not commit, push, change remotes, or add major dependencies without explicit
  approval. Preserve unrelated work.

See `docs/architecture.md` for contracts and future design boundaries.
