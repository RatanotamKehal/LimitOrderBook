#pragma once
#include <vector>
#include <unordered_map>
#include "types.h"

class LOB {
private:
	std::vector<PriceLevel> buy_orders;
	std::vector<PriceLevel> sell_orders;
	std::vector<Order> mem_pool;
	std::vector<OrderIndex> order_map;

public:
	void add(Order& order);

	void cancel(uint64_t order_id);
};

