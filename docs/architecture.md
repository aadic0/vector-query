# Architecture

## Ownership and representation

`TradeColumns` owns three vectors: `float` prices and `uint32_t` volumes and symbol
IDs. Unlike an array of structures, this structure of arrays places each field in
its own contiguous stream. A scan can load predicate columns before accessing
prices and future SIMD can load adjacent values from one column. This is a layout
choice to measure, not proof that it beats all row-oriented designs.

`TradeColumnView` carries three `span<const T>` values with no ownership or copying
of data. `TradeColumns::view()` creates it from an lvalue owner; calling `view()` on
a temporary is deleted. This prevents an easy lifetime mistake but cannot prevent
all dangling views. Owners must remain alive and storage stable for the entire
query. The same view and arbitrary half-open ranges can later be passed to SIMD
kernels and contiguous tasks without a storage abstraction hierarchy.

Both types report the price count as `size()`. Both expose consistency checks.
`execute_scalar` rejects unequal lengths with `invalid_argument`, then invalid
ranges with `out_of_range`, before entering the loop. Empty valid ranges work even
at the dataset end. Caller-provided prices must satisfy the documented finite,
non-negative contract; value checking is intentionally not repeated per query.

## Generation and execution

`generate_trades` validates all configuration bounds before allocating final-sized
vectors. A local `mt19937_64` and standard distributions generate values in a fixed
order. Volume endpoints are inclusive. Prices interpolate using double arithmetic
then round to float; the configured finite, non-negative float endpoints bound the
result. Double interpolation handles extreme float bounds without float overflow.
One symbol is valid; zero symbols, negative/nonfinite prices, and reversed bounds
are invalid even when the requested row count is zero. No global RNG state exists.
The determinism guarantee is scoped to one toolchain/library/floating environment.

The demo constructs an owner, creates a `Query`, obtains a view, and executes the
full range. The kernel tests `volume > minimum_volume` and exact symbol equality.
It adds matching float prices as double and increments a uint64 count. Fusing the
predicate and aggregation avoids an intermediate selection buffer, another pass,
and allocation in the hot path. There are no locks, logs, I/O, or virtual calls in
the per-row loop. Float storage saves space relative to double storage; double
accumulation reduces rounding error without recovering precision already lost
when a price was stored as float.

## Correctness reference and numerical comparisons

The small scalar implementation is the oracle for all later execution paths.
Hand-authored tests establish strict boundaries, conjunction semantics, subranges,
and exact counts independently of generated data. A mixed-magnitude example
exposes a float accumulator. Tests compare sums of the *stored float values*, not
ideal decimal values prior to float rounding.

For non-negative terms, sequential double summation has relative error bounded by
`gamma_n = n*epsilon / (1 - n*epsilon)` when `n*epsilon < 1`. Current tests use
`4*gamma_max(n,1)*max(1,abs(expected))` as an absolute tolerance, allowing reference
rounding with a small floor at zero. The handful of terms in these tests keep this
tight. Deterministic generator equality tests compare vectors exactly because they
check repeated generation, not arithmetic equivalence. Later reductions must
document and validate a suitable error bound for their accumulation order; counts
must always agree exactly.

## Benchmark boundary

Each Google Benchmark invocation allocates and generates data before entering its
State range loop. A reference scan obtains actual selectivity and warms the data.
Inside the loop only input barriers, the checked kernel call, and an output barrier
run. Destruction, counters, framework printing, and any JSON output happen outside
that loop. There is no file loading. Small cases include proportionally more
call/validation and benchmark-loop overhead. Default Release permits compiler
auto-vectorization; explicit SIMD has not been implemented.

The baseline reports rows/second across 12 KiB, 3 MiB, and 96 MiB logical working
sets with approximately 10% and 90% selectivity. It does not measure bandwidth
directly, flush caches, or prove cache boundaries. Any eventual performance claim
must account for measurement noise and compare equivalent data and query settings.

## Future boundaries — not implemented

An explicit AVX2 kernel will preserve this logical interface, arbitrary valid
ranges, and validation semantics. It will use eight-lane 32-bit loads, unsigned
integer comparison masks, combined predicates, conditional price accumulation,
match counting, a final horizontal reduction, and scalar tail handling. Multiple
accumulators require benchmark justification. Unsigned comparisons, especially
values above `INT32_MAX`, need care. Counts remain exact; sums use a defined
numerical tolerance. ISA flags must apply only to the relevant implementation.

The first parallel executor will have a fixed `std::jthread` worker pool, one
central bounded circular queue, a mutex, and `not_empty`/`not_full` condition
variables. Tasks represent fixed-size contiguous ranges. Each owns a unique
partial-result slot; a latch or equivalent tracks completion before one final
scalar reduction. Initially one query executes synchronously at a time.
`stop_token` is only for pool shutdown. A shared atomic floating-point accumulator
is excluded because it introduces contention and uncontrolled summation order.

Only after this baseline is measured should experiments add batched queue
operations, atomic next-task indices, worker-local deques, Chase–Lev stealing,
randomized victims, locality-aware stealing, cooperative cancellation, per-query
state, NUMA-local injectors, first-touch allocation, affinity, hierarchical
reduction, and adaptive task sizing. These designs must earn their complexity;
none is assumed to be faster.
