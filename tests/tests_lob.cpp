#include <gtest/gtest.h>
#include <random>
#include <vector>
#include "initial/lob.h"
#include "optimized/lob.h"

struct DualOrderTracker {
    uint64_t naive_id;
    uint64_t fast_id;
};

TEST(LOBTest, DifferentialFuzzTest) {
    initial::LOB naive_book;
    optimized::LOB fast_book;

    std::mt19937 rng(42);
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
            // ACTION: Add Limit Order
            uint32_t p = price_dist(rng);
            uint32_t q = qty_dist(rng);
            int s = side_dist(rng);

            initial::Order naive_order{ current_naive_id++, p, q, 0, initial::OrderType::Limit, static_cast<initial::Side>(s) };
            naive_book.add(naive_order);

            optimized::Order fast_order;
            fast_order.price = p; fast_order.quantity = q;
            fast_order.side = static_cast<optimized::Side>(s);
            fast_order.type = optimized::OrderType::Limit;
            auto fast_res = fast_book.add(fast_order);

            if (fast_res.order_id != optimized::INVALID_ORDER_ID) {
                active_orders.push_back({ naive_order.order_id, fast_res.order_id });
            }
        }
        else if (action < 80 && !active_orders.empty()) {
            // ACTION: Cancel a Random Resting Order
            std::uniform_int_distribution<size_t> index_dist(0, active_orders.size() - 1);
            size_t idx = index_dist(rng);
            DualOrderTracker target = active_orders[idx];

            bool naive_success = naive_book.cancel(initial::CancelRequest{ target.naive_id });
            bool fast_success = fast_book.cancel(target.fast_id);

            ASSERT_EQ(naive_success, fast_success) << "Divergence on cancel state at iteration " << i;

            active_orders[idx] = active_orders.back();
            active_orders.pop_back();
        }
        else {
            // ACTION: Add Market Order
            uint32_t q = qty_dist(rng);
            int s = side_dist(rng);

            initial::Order naive_market{ current_naive_id++, 0, q, 0, initial::OrderType::Market, static_cast<initial::Side>(s) };
            optimized::Order fast_market;
            fast_market.price = 0; fast_market.quantity = q;
            fast_market.side = static_cast<optimized::Side>(s);
            fast_market.type = optimized::OrderType::Market;

            auto naive_trades = naive_book.add(naive_market);
            auto fast_res = fast_book.add(fast_market);

            // INVARIANT 1: Market Order Execution Volumes Match
            uint32_t naive_filled = 0;
            for (const auto& t : naive_trades) { naive_filled += t.quantity; }
            uint32_t fast_filled = q - fast_res.remaining_quantity;

            ASSERT_EQ(naive_filled, fast_filled) << "Market Order execution volume divergence at iteration " << i;
        }
    }

    // ==========================================
    // INVARIANT 2: END OF TEST LIQUIDITY SWEEP
    // ==========================================

    // Drain Asks (Send a massive Buy)
    initial::Order drain_asks_naive{ current_naive_id++, 0, 10000000, 0, initial::OrderType::Market, initial::Side::Buy };
    optimized::Order drain_asks_fast;
    drain_asks_fast.price = 0; drain_asks_fast.quantity = 10000000; drain_asks_fast.side = optimized::Side::Buy; drain_asks_fast.type = optimized::OrderType::Market;

    uint32_t naive_asks_filled = 0;
    for (const auto& t : naive_book.add(drain_asks_naive)) { naive_asks_filled += t.quantity; }
    uint32_t fast_asks_filled = 10000000 - fast_book.add(drain_asks_fast).remaining_quantity;
    ASSERT_EQ(naive_asks_filled, fast_asks_filled) << "Final Ask liquidity mismatch!";

    // Drain Bids (Send a massive Sell)
    initial::Order drain_bids_naive{ current_naive_id++, 0, 10000000, 0, initial::OrderType::Market, initial::Side::Sell };
    optimized::Order drain_bids_fast;
    drain_bids_fast.price = 0; drain_bids_fast.quantity = 10000000; drain_bids_fast.side = optimized::Side::Sell; drain_bids_fast.type = optimized::OrderType::Market;

    uint32_t naive_bids_filled = 0;
    for (const auto& t : naive_book.add(drain_bids_naive)) { naive_bids_filled += t.quantity; }
    uint32_t fast_bids_filled = 10000000 - fast_book.add(drain_bids_fast).remaining_quantity;
    ASSERT_EQ(naive_bids_filled, fast_bids_filled) << "Final Bid liquidity mismatch!";
}