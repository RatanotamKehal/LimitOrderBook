# High-Performance C++ Limit Order Book & Matching Engine

A single-threaded C++20 matching engine to investigate techniques for low-latency order-book processing. 
This project measures the latency impact of replacing conventional tree-based data structures with contiguous memory pools, 
paged bitboard arrays, and cache-friendly records, that use no dynamic memory allocation on the matching path.

At a stress-test book depth of 10,000,000 resting orders, the optimized engine achieves a median **9.9 ns insert**, **3.6 ns cancel**, 
and **146 ns 5-level market sweep**. This delivers a 99.99% reduction in median cancellation latency compared to the `std::map` reference model.

## Experimental Design: Why This Architecture?

This project intentionally compares a reference model against successive data-oriented replacements to isolate and measure specific mechanical sympathy bottlenecks:

| Optimization | Target Problem | Data-Oriented Replacement | Primary Hardware Effect |
| :--- | :--- | :--- | :--- |
| `std::map` → **Paged Bitmaps** | Pointer chasing for price discovery | Array of 64-bit occupancy masks | Hardware-accelerated bit traversal |
| Heap Allocation → **Order Pool** | Allocation latency and jitter | Pre-allocated array + freelist | Allocation-free hot path |
| Queue Scan → **Generational Handle** | Cancellation lookup latency | Opaque handle routing | $O(1)$ direct slot addressing |
| Scattered Nodes → **32B Records** | Poor spatial locality | 32-byte contiguous records | Higher cache utilization |

## Architecture

```text
                      LIMIT ORDER BOOK
                             │
              ┌──────────────┴───────────────┐
              │                              │
     PAGED BITBOARD ARRAY               CONTIGUOUS POOL
              │                              │
      [Block 0: 64-bit Mask]           Slot 0: [32B Record]
      [Block 1: 64-bit Mask]           Slot 1: [32B Record]
      [Block N: 64-bit Mask]           Slot N: [32B Record]
              │                              ▲
    Linear block scan                        │
        + countr_zero()                      │
              │                              │
              └──────────────┬───────────────┘
                             │
                     ┌───────┴────────┐
                     │ Opaque 64-bit  │
                     │ Generational   │
                     │ Handle         │
                     └────────────────┘
```
**1. Pre-allocated Order Pool**  
The order pool reserves a prespecified amount of bytes, in this case, for a capacity of 10,000,000 order slots, excluding vector metadata. 
A custom freelist ensures rapid allocation/free logic, keeping the steady-state matching path free of dynamic memory allocation.  
  
**2. O(1) Order Addressing via Generational Handles**  
Each live order is referenced internally by an opaque 64-bit generational handle containing a 32-bit generation counter and a 32-bit pool index. 
The index provides direct $O(1)$ slot access, while the generation validates that the slot has not been reused since the handle was issued, 
rejecting stale references after slot reuse until generation-counter wraparound.  
  
**3. Cache-Friendly Data Layout**  
Orders are represented as contiguous 32-byte records, allowing exactly two records to occupy a 64-byte cache line when suitably aligned. 
This can prevent cache line straddling and pointer chasing, which improves spatial locality compared to node-based containers such as `std::list`.  
  
**4. Paged Bitset Price Discovery**  
Price levels are mapped into an array of 64-bit blocks. The engine iterates through the block array until a non-empty 64-bit occupancy is found. 
Once located, `std::countr_zero` allows the compiler to use the CPU's trailing-zero bit operation (e.g., `TZCNT`) to identify
the next occupied level within that block without sequentially scanning individual empty prices.  

# Performance
The following benchmarks measure the median latency of hot-path operations against a pre-populated book of 10,000,000 orders. 
Both engines are subjected to the same deterministic workload and price/quantity distributions.


| Operation | Baseline Median | Optimized Median | Speedup | Latency Reduction |
| :--- | ---: | ---: | ---: | ---: |
| Insert Resting Order | 484.0 ns | 9.9 ns | 48.8× | 97.9% |
| Cancel Order | 44.8 µs | 3.6 ns | 12,441× | 99.99% |
| 5-Level/50-Qty Sweep | 809.0 ns | 146.0 ns | 5.54× | 81.9% |

*Source: CPU Median times derived from batch averages*

📈 Scaling: Latency vs. Book Size
These benchmarks evaluate how each implementation behaves as the number of resting orders increases. 
The optimized engine maintains approximately constant measured latency over the tested range, while the pointer-heavy baseline 
degrades substantially as the working set grows.

| Initial Orders | Baseline Insert | Optimized Insert | Baseline Cancel | Optimized Cancel | Baseline Sweep | Optimized Sweep |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 40.1 ns | 9.8 ns | 32.1 ns | 3.6 ns | 781.0 ns | 153.0 ns |
| 1K | 51.6 ns | 10.0 ns | 47.9 ns | 3.5 ns | 854.0 ns | 154.0 ns |
| 10K | 52.3 ns | 9.8 ns | 64.2 ns | 3.5 ns | 837.0 ns | 157.0 ns |
| 100K | 164.6 ns | 9.8 ns | 219.4 ns | 3.6 ns | 872.0 ns | 154.0 ns |
| 1M | 302.8 ns | 9.5 ns | 1.64 µs | 3.4 ns | 753.0 ns | 153.0 ns |
| 10M | 484.0 ns | 9.9 ns | 44.8 µs | 3.6 ns | 809.0 ns | 146.0 ns |

*Source: Hardware benchmarking values across all 6 tested depths*

### Matching Semantics

This engine implements the core mechanics of a limit order book:
* **Price-Time Priority:** Strict price-time priority matching.
* **FIFO Execution:** First-In, First-Out (FIFO) execution within each individual price level.
* **Limit Orders:** Supports limit orders with resting remainders.
* **Market Orders:** Supports market orders with multi-level sweeps.
* **O(1) Cancellation:** Achieved via opaque generational handles.
* **Validation:** Built-in duplicate-order rejection.
* **Data Types:** Integer representation for precise price ticks and quantities.

**Not currently supported:** Order modification/replacement, time-in-force policies (IOC/FOK), stop orders, iceberg orders, and self-trade prevention.

### Correctness & Validation
Algorithmic parity is validated through differential testing against the reference model.

* **Deterministic Differential Testing:** A deterministic differential test fires 10,000 randomized operations at both engines using a fixed seed
 (Seed: 42). The internal state, cancellation execution, and resulting volumes are compared after each randomized operation.

* **Observable Event Auditing:** Rather than exposing private internal state via getters, the test suite relies on strict output equivalence.
 It verifies that execution volumes match perfectly between engines across all market orders and crossing limit orders.

* **End-of-Test Liquidity Sweep:** A final global liquidity sweep drains both books completely to verify that the remaining resting volumes are
  mathematically identical, this is to ensure that the lazy evaluation logic in the optimized implementation maintained perfect parity.

* **Execution Validation:** Each timed operation in the benchmark is required to succeed; failures terminate the benchmark
 rather than contributing a low latency. Benchmarks perform explicit runtime success checks with `if (!success) std::abort();`
 within the timed loops to prevent silent operation failures and compiler elimination.

### Benchmark Methodology
* **CPU:** Intel Core Ultra 7 155U
* **Logical Processors:** 12 Cores (2 P, 8 E, 2 LP-E) / 14 Threads
* **CPU Frequency:** Dynamic; 1.70GHz base. No fixed-frequency control was applied. Wall-clock latency reflects the CPU's runtime frequency and thermal state.
* **OS:** Windows 11
* **Compiler:** MSVC 19.4x
* **Build:** Release (C++20, /O2, /WX)
* **Framework:** Google Benchmark
* **Threads:** 1 (Single-threaded execution)
* **Workload:** Measurements represent steady-state operations against a pre-populated book. Each benchmark iteration processes a batch of 1,000 operations,
amortizing benchmark harness overhead across the measured operations. The reported median latency is derived from: `benchmarked
execution time / # of operations processed`
* **Cancellation Latency Definition:** Cancellation targets are newly inserted orders selected deterministically immediately before each timed batch.
Latency measures direct unlink/free of recently inserted orders using a pre-existing generational handle;
no order-ID lookup or handle construction is included in the timed operation.

### Scope & Limitations
* Single-threaded execution intended strictly for in-memory matching.

* Networking, serialization, matching-gateway, and IPC overhead are intentionally excluded.

* Benchmark results are inherently hardware-dependent and reflect x86-64 microarchitectures.

* The baseline implementation serves strictly as a mechanical reference model, not a production-ready exchange architecture.

* The lazy evaluation of `best_ask` and `best_bid` means external queries between active matches may return stale top-of-book prices if a cancellation empties the level.

* Further improvements for price level searches by implementing hierarchical bit-boards or Radix Trees

* OrderID can fall into an ABA-problem after the 32-bit integer `generation` wraps around

* `generation` will wrap around twice as fast as it is incremented by both add and cancel

### Future Work
* **Hierarchical Bitboards:** Implement multi-level bitboards or Radix Trees to optimize price level searches across a wider price band.

* **Live Market Replay:** Develop a WebSocket gateway to ingest live crypto exchange data to test the matching engine against the real-world.

### Build & Run
This project uses CMake, requires Ninja, and a compiler supporting C++20.
```text
# Clone the repository
git clone https://github.com/RatanotamK/LimitOrderBook.git
cd LimitOrderBook

# Configure and build via CMake
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja

# Run the correctness test suite
./lob_tests

# Run the microbenchmarks
./lob_benchmark
```
