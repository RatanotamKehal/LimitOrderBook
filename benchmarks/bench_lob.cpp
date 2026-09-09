#include <benchmark/benchmark.h>
#include <algorithm>
#include <vector>
#include "optimized/lob.h"
#include "initial/lob.h"

// --- Helper for Percentiles ---
static double p95(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    std::vector<double> copy = v;
    std::sort(copy.begin(), copy.end());
    return copy[static_cast<size_t>(copy.size() * 0.95)];
}

static double p99(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    std::vector<double> copy = v;
    std::sort(copy.begin(), copy.end());
    return copy[static_cast<size_t>(copy.size() * 0.99)];
}

#define DEPTH_BENCHMARK(func) \
    BENCHMARK(func) \
    ->RangeMultiplier(10)->Range(100, 10000000) \
    ->Repetitions(10) \
    ->ComputeStatistics("median", [](const std::vector<double>& v) { \
        std::vector<double> copy = v; \
        std::sort(copy.begin(), copy.end()); \
        return copy[copy.size() / 2]; \
    }) \
    ->ComputeStatistics("p95", p95) \
    ->ComputeStatistics("p99", p99) \
    ->ComputeStatistics("max", [](const std::vector<double>& v) { \
        return *std::max_element(v.begin(), v.end()); \
    })

// ==========================================
// SCENARIO 1: ISOLATED ADD
// ==========================================
static void BM_Initial_AddOnly_Depth(benchmark::State& state) {
    int depth = state.range(0);
    initial::LOB book;
    uint64_t current_id = 1;

    for (int i = 0; i < depth; ++i) {
        book.add(initial::Order{ current_id++, static_cast<initial::Price>(100 + (i % 100)), 10, 0, initial::OrderType::Limit, initial::Side::Buy });
    }

    initial::Order target{ 0, 150, 10, 0, initial::OrderType::Limit, initial::Side::Buy };

    while (state.KeepRunningBatch(1000)) {
        std::vector<uint64_t> ids_to_cancel;
        ids_to_cancel.reserve(1000);

        for (int i = 0; i < 1000; ++i) {
            target.order_id = current_id++;
            book.add(target);
            ids_to_cancel.push_back(target.order_id);
        }

        state.PauseTiming();
        for (uint64_t id : ids_to_cancel) {
            book.cancel(initial::CancelRequest{ id });
        }
        state.ResumeTiming();
    }
}
DEPTH_BENCHMARK(BM_Initial_AddOnly_Depth);

static void BM_Optimized_AddOnly_Depth(benchmark::State& state) {
    int depth = state.range(0);
    optimized::LOB book;

    for (int i = 0; i < depth; ++i) {
        optimized::Order dummy; dummy.price = 100 + (i % 100); dummy.quantity = 10;
        dummy.side = optimized::Side::Buy; dummy.type = optimized::OrderType::Limit;
        book.add(dummy);
    }

    optimized::Order target; target.price = 150; target.quantity = 10;
    target.side = optimized::Side::Buy; target.type = optimized::OrderType::Limit;

    while (state.KeepRunningBatch(1000)) {
        std::vector<uint64_t> ids_to_cancel;
        ids_to_cancel.reserve(1000);

        for (int i = 0; i < 1000; ++i) {
            ids_to_cancel.push_back(book.add(target).order_id);
        }

        state.PauseTiming();
        for (uint64_t id : ids_to_cancel) {
            book.cancel(id);
        }
        state.ResumeTiming();
    }
}
DEPTH_BENCHMARK(BM_Optimized_AddOnly_Depth);

// ==========================================
// SCENARIO 2: ISOLATED CANCEL
// ==========================================
static void BM_Initial_CancelOnly_Depth(benchmark::State& state) {
    int depth = state.range(0);
    initial::LOB book;
    uint64_t current_id = 1;

    for (int i = 0; i < depth; ++i) {
        book.add(initial::Order{ current_id++, static_cast<initial::Price>(100 + (i % 100)), 10, 0, initial::OrderType::Limit, initial::Side::Buy });
    }

    initial::Order target{ 0, 150, 10, 0, initial::OrderType::Limit, initial::Side::Buy };

    while (state.KeepRunningBatch(1000)) {
        state.PauseTiming();
        std::vector<uint64_t> ids_to_cancel;
        ids_to_cancel.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            target.order_id = current_id++;
            book.add(target);
            ids_to_cancel.push_back(target.order_id);
        }
        state.ResumeTiming();

        for (uint64_t id : ids_to_cancel) {
            book.cancel(initial::CancelRequest{ id });
        }
    }
}
DEPTH_BENCHMARK(BM_Initial_CancelOnly_Depth);

static void BM_Optimized_CancelOnly_Depth(benchmark::State& state) {
    int depth = state.range(0);
    optimized::LOB book;

    for (int i = 0; i < depth; ++i) {
        optimized::Order dummy; dummy.price = 100 + (i % 100); dummy.quantity = 10;
        dummy.side = optimized::Side::Buy; dummy.type = optimized::OrderType::Limit;
        book.add(dummy);
    }

    optimized::Order target; target.price = 150; target.quantity = 10;
    target.side = optimized::Side::Buy; target.type = optimized::OrderType::Limit;

    while (state.KeepRunningBatch(1000)) {
        state.PauseTiming();
        std::vector<uint64_t> ids_to_cancel;
        ids_to_cancel.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            ids_to_cancel.push_back(book.add(target).order_id);
        }
        state.ResumeTiming();

        for (uint64_t id : ids_to_cancel) {
            book.cancel(id);
        }
    }
}
DEPTH_BENCHMARK(BM_Optimized_CancelOnly_Depth);

// ==========================================
// SCENARIO 3: MARKET SWEEP
// ==========================================
static void BM_Initial_MatchSweep_Depth(benchmark::State& state) {
    int depth = state.range(0);
    initial::LOB book;
    uint64_t current_id = 1;

    for (int i = 0; i < depth; ++i) {
        book.add(initial::Order{ current_id++, static_cast<initial::Price>(100 + (i % 100)), 10, 0, initial::OrderType::Limit, initial::Side::Sell });
    }

    initial::Order market_buy{ current_id++, 0, 50, 0, initial::OrderType::Market, initial::Side::Buy };

    for (auto _ : state) {
        auto trades = book.add(market_buy);
        benchmark::DoNotOptimize(trades);

        state.PauseTiming();
        for (initial::Price p = 100; p < 105; ++p) {
            book.add(initial::Order{ current_id++, p, 10, 0, initial::OrderType::Limit, initial::Side::Sell });
        }
        market_buy.quantity = 50;
        market_buy.order_id = current_id++;
        state.ResumeTiming();
    }
}
DEPTH_BENCHMARK(BM_Initial_MatchSweep_Depth);

static void BM_Optimized_MatchSweep_Depth(benchmark::State& state) {
    int depth = state.range(0);
    optimized::LOB book;

    for (int i = 0; i < depth; ++i) {
        optimized::Order dummy; dummy.price = 100 + (i % 100); dummy.quantity = 10;
        dummy.side = optimized::Side::Sell; dummy.type = optimized::OrderType::Limit;
        book.add(dummy);
    }

    optimized::Order market_buy; market_buy.price = 0; market_buy.quantity = 50;
    market_buy.side = optimized::Side::Buy; market_buy.type = optimized::OrderType::Market;

    for (auto _ : state) {
        auto result = book.add(market_buy);
        benchmark::DoNotOptimize(result);

        state.PauseTiming();
        for (optimized::Price p = 100; p < 105; ++p) {
            optimized::Order ask; ask.price = p; ask.quantity = 10;
            ask.side = optimized::Side::Sell; ask.type = optimized::OrderType::Limit;
            book.add(ask);
        }
        market_buy.quantity = 50;
        state.ResumeTiming();
    }
}
DEPTH_BENCHMARK(BM_Optimized_MatchSweep_Depth);

BENCHMARK_MAIN();