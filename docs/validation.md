# VectorQuery milestone 1: verification report

Verified on 2026-10-19 UTC in `/home/aadi/Documents/Codex/2026-10-19/you-are-acting-as-a-senior`.

## Outcome

Implemented the scalar, single-threaded milestone. All four requested targets
compile in Debug, Release, and ASan/UBSan configurations. All 26 tests pass in each.
The demo and Release benchmarks run. No unresolved project-source compiler warnings
remain. The one unavailable acceptance check is plain `git diff --check`: this
workspace has no initialized Git repository. Git's no-index whitespace checks were
used instead. No commits, pushes, Git remote changes, global installations, or
writes outside the selected project were performed.

## Environment and preflight

- Ubuntu 25.10 (Questing Quokka), x86-64, KVM virtual machine.
- Exposed CPU: Intel Core i7-14650HX, four logical CPUs; AVX2 flag present.
- GCC / g++ 15.2.0, CMake 3.31.6, Ninja 1.12.1, clang-format 20.1.8, Git 2.51.0.
- CWD confirmed as above. No existing non-generated project files were found.
  The environment-managed `.git` directory is not an initialized repository;
  `git rev-parse --show-toplevel` and `git status --short --branch` could not return
  repository state. Existing environment placeholders were preserved.
- Read-only checks included `pwd`, `ls -la`, `rg --files --hidden -g '!.git' -g '!build' -g '!out'`,
  `git rev-parse --show-toplevel`, `git status --short --branch`, `uname -m`,
  `cat /etc/os-release`, `lscpu`, `cmake --version`, `ninja --version`,
  `c++ --version`, `command -v clang++ clang-format g++`, `clang-format --version`,
  and `git --version`. Ancestor AGENTS.md paths were checked; none supplied instructions.

## Files created

```text
CMakeLists.txt
CMakePresets.json
.gitignore
.clang-format
AGENTS.md
README.md
docs/architecture.md
docs/roadmap.md
docs/validation.md
include/vector_query/data/trade_columns.hpp
include/vector_query/data/data_generator.hpp
include/vector_query/query/query.hpp
include/vector_query/query/result.hpp
include/vector_query/kernels/scalar_kernel.hpp
src/data_generator.cpp
src/scalar_kernel.cpp
examples/run_query.cpp
tests/data_generator_tests.cpp
tests/scalar_kernel_tests.cpp
benchmarks/scalar_kernel_benchmark.cpp
```

`build/` contains generated builds and pinned dependency sources. `work/` contains
scratch scripts and verification logs. `outputs/` contains this report, raw benchmark
JSON, and a source archive. These directories are ignored by the supplied .gitignore.
No unrelated files were modified.

## Design decisions and code tour

1. `examples/run_query.cpp` configures 100,000 deterministic rows and a query.
2. `src/data_generator.cpp` validates bounds, allocates final-sized vectors, and
   fills them using a local mt19937_64 and standard distributions. Double
   interpolation protects extreme finite float price bounds. Determinism is
   guaranteed only within the same toolchain/library/floating-point environment.
3. `TradeColumns::view()` creates three read-only spans without copying data.
   Temporary-owner view creation is deleted; the owner must still outlive the view.
4. `src/scalar_kernel.cpp` first validates lengths and `[begin,end)`, then executes
   one fused predicate/aggregation loop. Volume comparison is strict, symbol
   comparison exact, prices are promoted to double, and counts are uint64_t.
5. The demo prints the returned result. Tests independently check hand-authored
   semantics, invalid inputs, generator invariants, unsigned extremes, and double
   accumulation. Sum tolerance is documented using a sequential error bound;
   counts are exact.
6. The benchmark generates data and computes actual selectivity before its timed
   loop. Inside are optimization barriers and the checked kernel call. No timed
   generation, allocation, I/O, or printing occurs. Three sizes and two thresholds
   measure warmed repeated scans with one symbol.

There is no explicit SIMD or concurrency implementation. Release leaves ordinary
compiler auto-vectorization enabled, with no global ISA flags or fast-math. The
future scalar/AVX2 and scheduler contracts are documented without placeholder
implementations or speculative interfaces.

## Exact build and execution commands

The following commands were executed from the project root (build/configure/test
output was redirected to matching `work/*.log` files). Debug configure was retried
after the network permission described below. The same build commands were repeated
after correcting the benchmark barrier warnings; no further rebuild was needed
for Release or sanitizers because they had already compiled the corrected source.

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

./build/release/vector_query_benchmarks --benchmark_min_time=0.2s --benchmark_repetitions=3 --benchmark_out=work/scalar-release.json --benchmark_out_format=json

clang-format -i include/vector_query/data/*.hpp include/vector_query/query/*.hpp include/vector_query/kernels/*.hpp src/*.cpp tests/*.cpp examples/*.cpp benchmarks/*.cpp
clang-format --dry-run --Werror include/vector_query/data/*.hpp include/vector_query/query/*.hpp include/vector_query/kernels/*.hpp src/*.cpp tests/*.cpp examples/*.cpp benchmarks/*.cpp

git diff --check
# Outside a repository, check each project file against the empty file:
git diff --no-index --check /dev/null CMakeLists.txt
# The same command was executed for each other project file by a Python loop.
```

Git no-index mode returns 1 for added files even with no whitespace diagnostics;
all checked files had empty diagnostics. Plain `git diff --check` returned 129
because no Git repository exists; it is **not** reported as passing.

## Observed checks

| Check | Result |
|---|---|
| Debug configure/build | Passed, all four targets |
| Release configure/build | Passed, all four targets |
| Sanitizer configure/build | Passed, all four targets |
| Debug CTest | 26/26 passed |
| Release CTest | 26/26 passed |
| ASan + UBSan CTest | 26/26 passed; no sanitizer diagnostics |
| Demo, Debug and Release | Passed, identical output below |
| Release benchmark | All six combinations, three repetitions each |
| Project warnings | None unresolved; initial barrier deprecations fixed |
| clang-format verification | Passed |
| Plain git diff --check | Unavailable: directory is not a Git repository |
| Git no-index whitespace validation | Passed for project files |

The sanitizer test preset sets `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. Project code is instrumented;
third-party libraries retain their own build settings. ThreadSanitizer is absent.

Observed demo output:

```text
Rows: 100000
Volume > 500; symbol_id = 3
Matches: 3105
Price sum: 157107.867070
```

## Initial scalar baseline

GCC Release (`-O3 -DNDEBUG`, C++23), no ISA-specific flags. Google Benchmark
v1.9.4, three repetitions with 0.2 seconds requested minimum per repetition. Timing
below is median CPU microseconds per full query, with median million rows/second.
CPU CV is sample standard deviation / mean across the three repetitions.

| Rows | Minimum volume | Actual matches | Median CPU us | Million rows/s | CPU CV |
|---:|---:|---:|---:|---:|---:|
| 1,024 | 100 | 91.016% | 0.450 | 2275.21 | 8.13% |
| 1,024 | 900 | 9.180% | 0.533 | 1921.64 | 8.97% |
| 262,144 | 100 | 90.056% | 299.600 | 874.98 | 1.52% |
| 262,144 | 900 | 9.959% | 278.497 | 941.28 | 2.84% |
| 8,388,608 | 100 | 89.994% | 11262.752 | 744.81 | 24.08% |
| 8,388,608 | 900 | 10.018% | 11265.853 | 744.60 | 17.02% |

Data uses seed 42, one symbol, prices in [1,100], and volumes in [1,1000]. Logical
working sets are 12 KiB, 3 MiB, and 96 MiB. These are not claims about cache residency
or measured DRAM bandwidth. Timed validation is included. The reference scan warms
data, and repeated iterations reuse it. Small-input results include call and
benchmark overhead.

The large-case CPU CV reaches about 24%; this is a noisy VM baseline, not statistical
proof of relative performance. Do not infer that either selectivity is faster from
these results. Frequency scaling, host activity, thermal throttling, virtualization,
and compiler changes all require control or documentation in future comparisons.
No speedup has been claimed, and no timing-driven kernel optimization was made.

## Issues encountered and limitations

- Initial FetchContent failed because restricted networking could not resolve
  GitHub. Network permission was requested and granted; the retry succeeded.
  Dependencies remain normal pinned FetchContent archives: GoogleTest v1.17.0
  and Google Benchmark v1.9.4. No replacement frameworks or global installs.
- `curl` was absent during network diagnosis. Python confirmed DNS failure; curl
  is not needed by the build. CMake performed the successful downloads.
- Google Benchmark's const-reference DoNotOptimize overload produced deprecation
  warnings on the first Debug build. Mutable local inputs/results use the supported
  overload now. Subsequent compilation was warning-free.
- No initialized Git repository means the ordinary diff acceptance check is still
  unavailable. The environment-managed placeholder was left untouched.
- Only this GCC/Linux environment was verified; the code does not claim cross-platform
  bitwise generator output. External callers must keep borrowed storage valid and
  supply finite non-negative prices. No nulls, parsing, runtime ISA dispatch, or
  parallel execution are implemented.

## Recommended next milestone

After explicit approval, implement an AVX2 kernel with scalar tail handling and
arbitrary-range support. Compare it against this scalar oracle using exact counts
and documented sum tolerance, including unsigned comparison extremes. Then benchmark
both under the same data and compiler configuration. No AVX2 work has begun.
