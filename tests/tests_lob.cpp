#include <gtest/gtest.h>
#include <random>
#include <vector>
#include "initial/lob.h"
#include "optimized/lob.h"

// Struct to link the IDs of the two engines for simultaneous cancellation
struct DualOrderTracker {
    uint64_t naive_id;
    uint64_t fast_id;
};

TEST(LOBTest, DifferentialFuzzTest) {
    initial::LOB naive_book;
    optimized::LOB fast_book;

    std::mt19937 rng(42); // Hardcoded seed guarantees the exact same chaos every run
    std::uniform_int_distribution<int> action_dist(0, 100);
    std::uniform_int_distribution<uint32_t> price_dist(100, 150);
    std::uniform_int_distribution<uint32_t> qty_dist(1, 50);
    std::uniform_int_distribution<int> side_dist(0, 1);

    std::vector<DualOrderTracker> active_orders;
    uint64_t current_naive_id = 1;

    const int ITERATIONS = 10000;

    for (int i = 0; i < ITERATIONS; ++i) {
        int action = action_dist(rng);

        if (action < 60) {
            // ACTION: Add Limit Order (60% probability)
            uint32_t p = price_dist(rng);
            uint32_t q = qty_dist(rng);
            int s = side_dist(rng);

            // Construct Naive
            initial::Order naive_order{ current_naive_id++, p, q, 0, initial::OrderType::Limit, static_cast<initial::Side>(s) };
            naive_book.add(naive_order);

            // Construct Fast
            optimized::Order fast_order;
            fast_order.price = p; fast_order.quantity = q;
            fast_order.side = static_cast<optimized::Side>(s);
            fast_order.type = optimized::OrderType::Limit;
            auto fast_res = fast_book.add(fast_order);

            // Track IDs for future cancellation testing
            if (fast_res.order_id != optimized::INVALID_ORDER_ID) {
                active_orders.push_back({ naive_order.order_id, fast_res.order_id });
            }

        }
        else if (action < 80 && !active_orders.empty()) {
            // ACTION: Cancel a Random Resting Order (20% probability)
            std::uniform_int_distribution<size_t> index_dist(0, active_orders.size() - 1);
            size_t idx = index_dist(rng);
            DualOrderTracker target = active_orders[idx];

            bool naive_success = naive_book.cancel(initial::CancelRequest{ target.naive_id });
            bool fast_success = fast_book.cancel(target.fast_id);

            // The bitboard and the map must agree on whether the order was cancellable
            ASSERT_EQ(naive_success, fast_success) << "Divergence on cancel state at iteration " << i;

            // Remove from tracking list by swapping with the back and popping
            active_orders[idx] = active_orders.back();
            active_orders.pop_back();

        }
        else {
            // ACTION: Add Market Order (20% probability)
            uint32_t q = qty_dist(rng);
            int s = side_dist(rng);

            initial::Order naive_market{ current_naive_id++, 0, q, 0, initial::OrderType::Market, static_cast<initial::Side>(s) };
            optimized::Order fast_market;
            fast_market.price = 0; fast_market.quantity = q;
            fast_market.side = static_cast<optimized::Side>(s);
            fast_market.type = optimized::OrderType::Market;

            // We do not check trades sizes strictly here because the naive implementation 
            // returns executed trades, while the optimized returns remaining volume. 
            // We just ensure the engine doesn't crash during deep sweeps.
            naive_book.add(naive_market);
            fast_book.add(fast_market);
        }
    }
}