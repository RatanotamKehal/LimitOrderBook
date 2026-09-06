#include "lob.h"
#include <cmath>
#include <limits>

constexpr OrderIndex INVALID_INDEX = std::numeric_limits<OrderIndex>::max();
constexpr int MEM_POOL_SIZE{ 1000000 };
const int ORDER_POOL_SIZE{ std::pow(2, std::log2(MEM_POOL_SIZE)) };

LOB::LOB() {
	buy_orders.resize(ORDER_POOL_SIZE, { INVALID_INDEX, INVALID_INDEX });
	sell_orders.resize(ORDER_POOL_SIZE, { INVALID_INDEX, INVALID_INDEX });
	mem_pool.resize(MEM_POOL_SIZE);
	order_map.resize(MEM_POOL_SIZE, INVALID_INDEX);

	free_list_head = 0;

	for (int i = 0; i < MEM_POOL_SIZE; i++) {
		mem_pool[i].next = i + 1;
	}
	mem_pool[MEM_POOL_SIZE - 1].next = INVALID_INDEX;

	best_bid = 0;
	best_ask = std::numeric_limits<Price>::max();
}

void LOB::add(Order& order) {
	
}

void LOB::cancel(uint64_t order_id) {

}
