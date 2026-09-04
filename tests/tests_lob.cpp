#include <gtest/gtest.h>
#include "lob.h"


namespace lob {
    Order createOrder(Price price, Quantity quantity, Side side,
                      OrderType type = OrderType::Limit) {
        static OrderID id = 1;

        return Order{
            id++,
            price,
            quantity,
            0,
            type,
            side,
        };
    }

    TEST(LOBTest, RestingOrders) {
        LOB book;
        Order buy_order = createOrder(100, 50, Side::Buy);
        Order sell_order = createOrder(105, 50, Side::Sell);

        std::vector<Trade> trades1 = book.add(buy_order);
        std::vector<Trade> trades2 = book.add(sell_order);

        EXPECT_TRUE(trades1.empty());
        EXPECT_TRUE(trades2.empty());
    }

    TEST(LOBTest, ExactFill) {
        LOB book;
        Order buy_order = createOrder(100, 100, Side::Buy);
        Order sell_order = createOrder(100, 100, Side::Sell);

        book.add(buy_order);
        std::vector<Trade> trade = book.add(sell_order);

        EXPECT_TRUE(trade.size() == 1);
    }

    TEST(LOBTest, PartialFillBuy) {
        LOB book;
        Order buy_order = createOrder(100, 100, Side::Buy);
        Order sell_order = createOrder(100, 50, Side::Sell);
        book.add(sell_order);
        std::vector<Trade> trade = book.add(buy_order);

        EXPECT_TRUE(trade.size() == 1);
        EXPECT_TRUE(book.has_order(buy_order.order_id));
    }

    TEST(LOBTest, PartialFillSell) {
        LOB book;
        Order buy_order = createOrder(100, 1, Side::Buy);
        Order sell_order = createOrder(100, 51, Side::Sell);
        book.add(buy_order);
        std::vector<Trade> trade = book.add(sell_order);

        EXPECT_EQ(trade.size(), 1);
        EXPECT_TRUE(book.has_order(sell_order.order_id));
    }

    TEST(LOBTest, AddZero) {
        LOB book;

        Order buy_order = createOrder(100, 0, Side::Buy);
        Order sell_order = createOrder(100, 0, Side::Sell);

        EXPECT_FALSE(book.has_order(buy_order.order_id));
        EXPECT_FALSE(book.has_order(sell_order.order_id));
    }   

    TEST(LOBTest, PriceTimePriority) {
        LOB book;
        Order sell_order1 = createOrder(101, 10, Side::Sell);
        Order sell_order2 = createOrder(102, 10, Side::Sell);
        Order sell_order3 = createOrder(103, 10, Side::Sell);
        Order buy_order = createOrder(0, 25, Side::Buy, OrderType::Market);
        // Note that for a market order price is irrelevant

        book.add(sell_order1);
        book.add(sell_order2);
        book.add(sell_order3);
        std::vector<Trade> trade = book.add(buy_order);

        EXPECT_TRUE(trade.size() == 3);
        EXPECT_TRUE(trade.at(0).quantity == 10);
        EXPECT_TRUE(trade.at(1).quantity == 10);
        EXPECT_TRUE(trade.at(2).quantity == 5);
    }

    TEST(LOBTest, Cancellations) {
        LOB book;
        Order sell_order = createOrder(101, 10, Side::Sell);
        CancelRequest cancel_request{ sell_order.order_id };

        book.add(sell_order);

        bool cancelled = book.cancel(cancel_request);
        bool cancelled_failure = book.cancel(cancel_request);
        
        EXPECT_TRUE(cancelled);
        EXPECT_FALSE(cancelled_failure);
    }
}
