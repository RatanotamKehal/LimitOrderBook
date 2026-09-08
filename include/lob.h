#pragma once
#include <vector>
#include <limits>
#include "types.h"

inline constexpr uint64_t PRICE_MULTIPLIER = 10000; // 10,000

inline constexpr OrderIndex INVALID_INDEX = std::numeric_limits<OrderIndex>::max();
inline constexpr uint64_t INVALID_ORDER_ID = std::numeric_limits<uint64_t>::max();

inline constexpr uint64_t MEM_POOL_SIZE{ 1 << 20 }; // 1,048,576
inline constexpr uint64_t ORDER_POOL_SIZE{ 1 << 19 }; // 524,288

inline constexpr Price MAX_PRICE = ORDER_POOL_SIZE - 1;
inline constexpr Price MIN_PRICE = 0;

inline constexpr OrderID makeOrderID(OrderIndex index, Generation gen) {
	return (static_cast<OrderID>(gen) << 32) | index;
}
inline constexpr OrderIndex getOrderIndex(OrderID id) {
	return static_cast<OrderIndex>(id);
}

inline constexpr Generation getGeneration(OrderID id) {
	return static_cast<Generation>(id >> 32);
}

class LOB {
private:
	std::vector<PriceLevel> buy_orders;
	std::vector<PriceLevel> sell_orders;
	std::vector<uint64_t> buy_bitvector;
	std::vector<uint64_t> sell_bitvector;

	std::vector<Order> mem_pool;
	OrderIndex free_list_head;

	Price best_bid;
	Price best_ask;

	bool has_bids;
	bool has_asks;

	void setSellBit(Price price);
	void setBuyBit(Price price);
	void clearSellBit(Price price);
	void clearBuyBit(Price price);

	Quantity matchAgainstAsks(Quantity quantity, Price limit_price);
	Quantity matchAgainstBids(Quantity quantity, Price limit_price);
	AddResult addRemainingToList(Order& order);

	void freeOrder(OrderIndex index);


public:
	LOB();

	AddResult add(Order& order);
	bool cancel(OrderID order_id);
};

