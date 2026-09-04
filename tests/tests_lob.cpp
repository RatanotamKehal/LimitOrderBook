#include <gtest/gtest.h>
#include "lob.h"


namespace lob {
    Order createOrder(Price price, Quantity quantity, Side side,
                      OrderType type = OrderType::Limit) {
        static OrderId id = 1;

        return Order{
            id++,
            price,
            quantity,
            0,
            side,
            type,
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

}
