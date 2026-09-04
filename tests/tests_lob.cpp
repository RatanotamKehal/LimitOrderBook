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

}
