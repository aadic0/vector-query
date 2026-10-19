# Roadmap

Build and measure in this order:

1. Project infrastructure and deterministic generated data — milestone 1.
2. Scalar filtering and aggregation baseline — milestone 1.
3. Explicit AVX2 kernel with scalar tail handling.
4. Scalar-versus-AVX2 correctness and performance comparison.
5. Fixed-size task representation.
6. Mutex-protected bounded circular queue.
7. Fixed `std::jthread` worker pool.
8. Query coordinator using fixed-size chunks.
9. Per-task partial-result slots and final reduction.
10. Thread-count, chunk-size, dataset-size, and selectivity experiments.
11. Batched or atomic-index scheduler experiments.
12. Per-worker deques and work stealing.
13. Multiple concurrent queries, cancellation, and error propagation.
14. NUMA-aware data placement and scheduling.
15. Adaptive task granularity.
16. Richer predicates, aggregations, and external datasets.

## Initial V1 scope

V1 is larger than the current scalar milestone. It includes scalar and AVX2 kernels,
a fixed thread pool, a central mutex-protected bounded queue, fixed-size chunks,
one query at a time, a synchronous result, per-task partial results, and a final
scalar reduction. Stop tokens initially govern pool shutdown only.

## Post-V1

Lock-free queues, work stealing, multiple simultaneous queries, query cancellation,
priorities and fairness, NUMA-aware scheduling, adaptive chunk sizes, runtime
AVX2/AVX-512 dispatch, dictionary-encoded strings, and SIMD CSV parsing are deferred.
Batched and atomic-index experiments follow measurement of the central-queue V1.

Every stage needs correctness against the scalar reference, comparable workloads,
and measured evidence. No speedup is presumed. Start the explicit AVX2 phase only
after the project owner approves it.
