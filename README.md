# VectorQuery

An in-memory, column-oriented analytical executor for studying CPU caches, SIMD,
selectivity, task granularity, scheduling, and memory bandwidth through controlled
experiments. Every optimization starts from a correct, measurable baseline.

**Current status: milestone 1 — scalar and single-threaded.** No explicit SIMD,
thread pool, queue, CSV loader, or NUMA implementation exists yet. No speedup is
claimed. The library has no third-party runtime dependency.

```sql
SELECT SUM(price), COUNT(*)
FROM trades
WHERE volume > minimum_volume AND symbol_id = target_symbol;
```

Prices are `float`; sums are `double`; counts are `uint64_t`. Volume comparison is
strictly greater than and symbol comparison is exact equality. There are no nulls.
Generated prices are finite and non-negative. Ranges are half-open `[begin, end)`;
empty ranges return zero. Column lengths and ranges are checked before scanning.

## Design and layout

Separate contiguous vectors store prices, volumes, and symbol IDs. This avoids
interleaved-record padding and makes each column independently streamable. The
scalar kernel borrows read-only spans and fuses filtering with aggregation, without
allocating an intermediate selection or result column.

```text
include/vector_query/   Public data, query, result, and kernel types
src/                   Deterministic generator and scalar implementation
examples/              Small deterministic query demo
tests/                GoogleTest semantic and generator tests
benchmarks/            Google Benchmark scalar baseline
docs/                  Architecture, roadmap, and observed verification
```

## Prerequisites

CMake 3.25+, Ninja, and a C++23 compiler and standard library. Verified environment
and results are recorded in [validation.md](docs/validation.md). GCC or Clang with
AddressSanitizer and UndefinedBehaviorSanitizer is required for the sanitizer preset.
clang-format 20 was used for formatting; it is not required for compilation.

CMake FetchContent downloads GoogleTest **v1.17.0** and Google Benchmark **v1.9.4**
from explicit release tags. Initial configuration needs HTTPS access to GitHub
and its archive download host. Nothing is installed globally. Dependencies live
under `build/<preset>/_deps`. For an offline build with already downloaded matching
sources, pass `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest` and
`-DFETCHCONTENT_SOURCE_DIR_GOOGLEBENCHMARK=/path/to/benchmark` when configuring.

## Build, test, and run

Run these commands from the project root:

```sh
cmake --preset debug
cmake --build --preset debug --parallel 2
ctest --preset debug
./build/debug/vector_query_demo

cmake --preset release
cmake --build --preset release --parallel 2
ctest --preset release
./build/release/vector_query_demo

cmake --preset sanitizers
cmake --build --preset sanitizers --parallel 2
ctest --preset sanitizers
```

All four targets build in each preset: `vector_query`, `vector_query_demo`,
`vector_query_tests`, and `vector_query_benchmarks`. Compile commands are exported
into each build directory. Warnings and sanitizers are target-scoped to project
code. ASan/UBSan are combined; TSan is not enabled. Sanitized binaries are for
correctness checks, not performance measurements.

The demo uses 100,000 rows, seed 42, sixteen symbols, and a volume threshold of
500. Output has this form (exact generated values depend on the standard library):

```text
Rows: 100000
Volume > 500; symbol_id = 3
Matches: 3105
Price sum: 157107.867070
```

## Benchmark baseline

```sh
./build/release/vector_query_benchmarks --benchmark_min_time=0.2s --benchmark_repetitions=3
# Optional machine-readable results (create work/ first if necessary):
mkdir -p work
./build/release/vector_query_benchmarks --benchmark_min_time=0.2s --benchmark_repetitions=3 --benchmark_out=work/scalar-release.json --benchmark_out_format=json
```

Sizes are 1,024, 262,144, and 8,388,608 rows: approximately 12 KiB, 3 MiB, and
96 MiB of logical column storage. These represent small, medium, and large working
sets; they do not guarantee particular cache residency on every machine. Volumes
are uniform integers from 1 to 1,000. A single symbol matches every row's symbol
predicate; thresholds of 100 and 900 yield approximately 90% and 10% overall
selectivity. `match_fraction` reports the actual fraction. This isolates volume
selectivity; it does not yet measure multi-symbol distributions or correlations.

`items_per_second` means rows scanned per second, not matches per second. Setup,
allocation, generation, a reference scan, and reporting occur outside the timed
range loop. The timed region includes the kernel call and its invariant checks.
Results and inputs use Google Benchmark optimization barriers. Repeated scans
reuse the same data; this is a warmed, steady-state experiment, not a cold-cache
measurement. Twelve logical bytes per row are not measured DRAM traffic: conditional
loads and caching can change actual traffic substantially.

Release uses normal compiler optimization without global ISA flags, fast-math,
or manual SIMD. The compiler may auto-vectorize; “scalar” describes the source
implementation, not a promise about generated instructions. Future explicit AVX2
results must record compiler flags and inspect generated code as appropriate.
CPU frequency scaling, other processes, thermal throttling, virtualization,
and compiler configuration all affect timings. Repeat measurements and report
variation; one run is not statistical certainty.

## Limits and next steps

The generator uses local `mt19937_64` state and standard distributions. A repeated
configuration produces identical vectors within the same supported toolchain,
standard library, and floating-point environment. Distribution mappings and
floating-point rounding may vary across implementations: cross-platform bitwise
reproducibility is not promised. Price bounds are inclusive after conversion to
float (rounding may reach the upper endpoint); volume bounds are inclusive.
Invalid configurations throw `invalid_argument`, even for zero rows. Allocation
failures propagate as standard allocation/length exceptions.

Views do not own memory: the owner must outlive the call and its vectors must not
be resized or concurrently modified while borrowed. `size()` reports price-column
length; call `has_consistent_sizes()` to validate all lengths. The kernel validates
shape and range, but does not scan for NaNs, negative prices, or infinities: external
callers must supply finite non-negative prices. Query symbols outside the generated
dictionary simply match nothing. Floating-point accumulation order may change in
future kernels, so comparisons use an explicit error tolerance and exact counts.

Read [architecture](docs/architecture.md) and the staged [roadmap](docs/roadmap.md).
