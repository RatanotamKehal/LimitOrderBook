#include <benchmark/benchmark.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cassert>
#include "optimized/lob.h"
#include "initial/lob.h"

// --- Safe Interpolated Percentiles ---
static double percentile(const std::vector<double>& v, double p) {
    if (v.empty()) return 0.0;
    std::vector<double> copy = v;
    std::sort(copy.begin(), copy.end());
    const size_t index = static_cast<size_t>(std::ceil(p * copy.size())) - 1;
    return copy[std::min(index, copy.size() - 1)];
}

static double p95(const std::vector<double>& v) { return percentile(v, 0.95); }
static double p99(const std::vector<double>& v) { return percentile(v, 0.99); }

#define HFT_BENCHMARK(func) \
    BENCHMARK(func) \
    ->RangeMultiplier(10)->Range(100, 10000000) \
    ->Repetitions(30) \
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

// --- Deterministic Constants ---
constexpr std::size_t NUM_PRICE_LEVELS = 1000;
constexpr uint32_t ORDER_QTY = 10;

// ==========================================
// SCENARIO 1: ISOLATED ADD
// ==========================================
static void BM_Initial_Add(benchmark::State& state) {
    const std::size_t initial_orders = static_cast<std::size_t>(state.range(0));
    initial::LOB book;
    uint64_t current_id = 1;

    for (std::size_t i = 0; i < initial_orders; ++i) {
        book.add(initial::Order{ current_id++, static_cast<initial::Price>(100 + (i % NUM_PRICE_LEVELS)), ORDER_QTY, 0, initial::OrderType::Limit, initial::Side::Buy });
    }

    initial::Order target{ 0, 150, ORDER_QTY, 0, initial::OrderType::Limit, initial::Side::Buy };
    std::vector<uint64_t> ids;
    ids.reserve(1000);

    for (auto _ : state) {
        ids.clear();
        for (int i = 0; i < 1000; ++i) {
            target.order_id = current_id++;
            book.add(target);
            ids.push_back(target.order_id);
        }

        state.PauseTiming();
        for (uint64_t id : ids) {
            bool success = book.cancel(initial::CancelRequest{ id });
            assert(success); // Correctness validation
        }
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}
HFT_BENCHMARK(BM_Initial_Add);

static void BM_Optimized_Add(benchmark::State& state) {
    const std::size_t initial_orders = static_cast<std::size_t>(state.range(0));
    optimized::LOB book;

    for (std::size_t i = 0; i < initial_orders; ++i) {
        optimized::Order dummy; dummy.price = 100 + (i % NUM_PRICE_LEVELS); dummy.quantity = ORDER_QTY;
        dummy.side = optimized::Side::Buy; dummy.type = optimized::OrderType::Limit;
        book.add(dummy);
    }

    optimized::Order target; target.price = 150; target.quantity = ORDER_QTY;
    target.side = optimized::Side::Buy; target.type = optimized::OrderType::Limit;
    std::vector<uint64_t> ids;
    ids.reserve(1000);

    for (auto _ : state) {
        ids.clear();
        for (int i = 0; i < 1000; ++i) {
            auto result = book.add(target);
            benchmark::DoNotOptimize(result);
            ids.push_back(result.order_id);
        }

        state.PauseTiming();
        for (uint64_t id : ids) {
            bool success = book.cancel(id);
            assert(success); // Correctness validation
        }
        state.ResumeTiming();
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}
HFT_BENCHMARK(BM_Optimized_Add);

// ==========================================
// SCENARIO 2: ISOLATED CANCEL
// ==========================================
static void BM_Initial_Cancel(benchmark::State& state) {
    const std::size_t initial_orders = static_cast<std::size_t>(state.range(0));
    initial::LOB book;
    uint64_t current_id = 1;

    for (std::size_t i = 0; i < initial_orders; ++i) {
        book.add(initial::Order{ current_id++, static_cast<initial::Price>(100 + (i % NUM_PRICE_LEVELS)), ORDER_QTY, 0, initial::OrderType::Limit, initial::Side::Buy });
    }

    initial::Order target{ 0, 150, ORDER_QTY, 0, initial::OrderType::Limit, initial::Side::Buy };
    std::vector<uint64_t> ids;
    ids.reserve(1000);

    for (auto _ : state) {
        state.PauseTiming();
        ids.clear();
        for (int i = 0; i < 1000; ++i) {
            target.order_id = current_id++;
            book.add(target);
            ids.push_back(target.order_id);
        }
        state.ResumeTiming();

        for (uint64_t id : ids) {
            bool success = book.cancel(initial::CancelRequest{ id });
            assert(success);
            benchmark::DoNotOptimize(success);
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}
HFT_BENCHMARK(BM_Initial_Cancel);

static void BM_Optimized_Cancel(benchmark::State& state) {
    const std::size_t initial_orders = static_cast<std::size_t>(state.range(0));
    optimized::LOB book;

    for (std::size_t i = 0; i < initial_orders; ++i) {
        optimized::Order dummy; dummy.price = 100 + (i % NUM_PRICE_LEVELS); dummy.quantity = ORDER_QTY;
        dummy.side = optimized::Side::Buy; dummy.type = optimized::OrderType::Limit;
        book.add(dummy);
    }

    optimized::Order target; target.price = 150; target.quantity = ORDER_QTY;
    target.side = optimized::Side::Buy; target.type = optimized::OrderType::Limit;
    std::vector<uint64_t> ids;
    ids.reserve(1000);

    for (auto _ : state) {
        state.PauseTiming();
        ids.clear();
        for (int i = 0; i < 1000; ++i) {
            auto result = book.add(target);
            ids.push_back(result.order_id);
        }
        state.ResumeTiming();

        for (uint64_t id : ids) {
            bool success = book.cancel(id);
            assert(success);
            benchmark::DoNotOptimize(success);
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}
HFT_BENCHMARK(BM_Optimized_Cancel);

// ==========================================
// SCENARIO 3: MARKET SWEEP (5 Distinct Levels)
// ==========================================
static void BM_Initial_MatchSweep(benchmark::State& state) {
    const std::size_t initial_orders = static_cast<std::size_t>(state.range(0));
    initial::LOB book;
    uint64_t current_id = 1;

    for (std::size_t i = 0; i < initial_orders; ++i) {
        book.add(initial::Order{ current_id++, static_cast<initial::Price>(500 + (i % NUM_PRICE_LEVELS)), ORDER_QTY, 0, initial::OrderType::Limit, initial::Side::Sell });
    }

    initial::Order market_buy{ 0, 0, 50, 0, initial::OrderType::Market, initial::Side::Buy };

    for (auto _ : state) {
        state.PauseTiming();
        for (initial::Price p = 100; p < 105; ++p) {
            book.add(initial::Order{ current_id++, p, ORDER_QTY, 0, initial::OrderType::Limit, initial::Side::Sell });
        }
        market_buy.quantity = 50;
        market_buy.order_id = current_id++;
        state.ResumeTiming();

        auto trades = book.add(market_buy);
        benchmark::DoNotOptimize(trades);
    }
    state.SetItemsProcessed(state.iterations());
}
HFT_BENCHMARK(BM_Initial_MatchSweep);

static void BM_Optimized_MatchSweep(benchmark::State& state) {
    const std::size_t initial_orders = static_cast<std::size_t>(state.range(0));
    optimized::LOB book;

    for (std::size_t i = 0; i < initial_orders; ++i) {
        optimized::Order dummy; dummy.price = 500 + (i % NUM_PRICE_LEVELS); dummy.quantity = ORDER_QTY;
        dummy.side = optimized::Side::Sell; dummy.type = optimized::OrderType::Limit;
        book.add(dummy);
    }

    optimized::Order market_buy; market_buy.price = 0; market_buy.quantity = 50;
    market_buy.side = optimized::Side::Buy; market_buy.type = optimized::OrderType::Market;

    for (auto _ : state) {
        state.PauseTiming();
        for (optimized::Price p = 100; p < 105; ++p) {
            optimized::Order ask; ask.price = p; ask.quantity = ORDER_QTY;
            ask.side = optimized::Side::Sell; ask.type = optimized::OrderType::Limit;
            book.add(ask);
        }
        market_buy.quantity = 50;
        state.ResumeTiming();

        auto result = book.add(market_buy);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}
HFT_BENCHMARK(BM_Optimized_MatchSweep);

BENCHMARK_MAIN();