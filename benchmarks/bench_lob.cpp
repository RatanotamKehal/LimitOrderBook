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

// Custom Macro to apply Repetitions and Stats universally
#define HFT_BENCHMARK(func) \
    BENCHMARK(func) \
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

// ==========================================
// SCENARIO 1: Adding a Resting Order
// ==========================================
static void BM_Initial_AddResting(benchmark::State& state) {
    initial::LOB book;
    initial::Order order{ 1, 100, 10, 0, initial::OrderType::Limit, initial::Side::Buy };
    for (auto _ : state) {
        book.add(order);
        state.PauseTiming();
        book.cancel(initial::CancelRequest{ 1 });
        state.ResumeTiming();
    }
}
HFT_BENCHMARK(BM_Initial_AddResting);

static void BM_Optimized_AddResting(benchmark::State& state) {
    optimized::LOB book;
    optimized::Order order;
    order.price = 100; order.quantity = 10;
    order.side = optimized::Side::Buy; order.type = optimized::OrderType::Limit;
    for (auto _ : state) {
        auto result = book.add(order);
        benchmark::DoNotOptimize(result);
        state.PauseTiming();
        book.cancel(result.order_id);
        state.ResumeTiming();
    }
}
HFT_BENCHMARK(BM_Optimized_AddResting);

// ==========================================
// SCENARIO 2: O(1) Cancellation
// ==========================================
static void BM_Initial_Cancel(benchmark::State& state) {
    initial::LOB book;
    initial::Order order{ 1, 100, 10, 0, initial::OrderType::Limit, initial::Side::Buy };
    for (auto _ : state) {
        state.PauseTiming();
        book.add(order);
        state.ResumeTiming();
        bool success = book.cancel(initial::CancelRequest{ 1 });
        benchmark::DoNotOptimize(success);
    }
}
HFT_BENCHMARK(BM_Initial_Cancel);

static void BM_Optimized_Cancel(benchmark::State& state) {
    optimized::LOB book;
    optimized::Order order;
    order.price = 100; order.quantity = 10;
    order.side = optimized::Side::Buy; order.type = optimized::OrderType::Limit;
    for (auto _ : state) {
        state.PauseTiming();
        auto result = book.add(order);
        state.ResumeTiming();
        bool success = book.cancel(result.order_id);
        benchmark::DoNotOptimize(success);
    }
}
HFT_BENCHMARK(BM_Optimized_Cancel);

// ==========================================
// SCENARIO 3: Aggressive Market Sweep (5 Levels)
// ==========================================
static void BM_Initial_MarketSweep(benchmark::State& state) {
    initial::LOB book;
    for (initial::Price p = 100; p < 105; ++p) {
        book.add(initial::Order{ p, p, 10, 0, initial::OrderType::Limit, initial::Side::Sell }); // Using p as ID
    }
    initial::Order market_buy{ 999, 0, 50, 0, initial::OrderType::Market, initial::Side::Buy };
    for (auto _ : state) {
        auto trades = book.add(market_buy);
        benchmark::DoNotOptimize(trades);
        state.PauseTiming();
        for (initial::Price p = 100; p < 105; ++p) {
            book.add(initial::Order{ p, p, 10, 0, initial::OrderType::Limit, initial::Side::Sell });
        }
        market_buy.quantity = 50;
        state.ResumeTiming();
    }
}
HFT_BENCHMARK(BM_Initial_MarketSweep);

static void BM_Optimized_MarketSweep(benchmark::State& state) {
    optimized::LOB book;
    for (optimized::Price p = 100; p < 105; ++p) {
        optimized::Order ask;
        ask.price = p; ask.quantity = 10;
        ask.side = optimized::Side::Sell; ask.type = optimized::OrderType::Limit;
        book.add(ask);
    }
    optimized::Order market_buy;
    market_buy.price = 0; market_buy.quantity = 50;
    market_buy.side = optimized::Side::Buy; market_buy.type = optimized::OrderType::Market;
    for (auto _ : state) {
        auto result = book.add(market_buy);
        benchmark::DoNotOptimize(result);
        state.PauseTiming();
        for (optimized::Price p = 100; p < 105; ++p) {
            optimized::Order ask;
            ask.price = p; ask.quantity = 10;
            ask.side = optimized::Side::Sell; ask.type = optimized::OrderType::Limit;
            book.add(ask);
        }
        market_buy.quantity = 50;
        state.ResumeTiming();
    }
}
HFT_BENCHMARK(BM_Optimized_MarketSweep);

BENCHMARK_MAIN();