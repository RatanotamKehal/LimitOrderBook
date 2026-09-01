#pragma once
#include <map>
#include <deque>
#include "types.h"

class LOB {
private:
	std::map<Price, std::deque<Order>> buy_orders;
	std::map<Price, std::deque<Order>> sell_orders;

public:
	void add(Order& order);

	void cancel(uint64_t order_id);
};

