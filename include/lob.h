#pragma once
#include <vector>
#include <unordered_map>
#include "types.h"

class LOB {
private:
	std::vector<PriceLevel> buy_orders;
	std::vector<PriceLevel> sell_orders;
	std::vector<uint64_t> buy_bitvector;
	std::vector<uint64_t> sell_bitvector;

	std::vector<Order> mem_pool;
	std::vector<OrderIndex> order_map;

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
	void addRemainingToList(Order& order);

public:
	LOB();

	Quantity add(Order& order);
	bool cancel(uint64_t order_id);
};

