#include "lob.h"
#include <limits>

constexpr OrderIndex INVALID_INDEX = std::numeric_limits<OrderIndex>::max();
constexpr Price MAX_PRICE = std::numeric_limits<Price>::max();
constexpr Price MIN_PRICE = 0;

constexpr uint64_t MEM_POOL_SIZE{ 1 << 20 }; // 1,048,576
constexpr uint64_t ORDER_POOL_SIZE{ 1 << 19 }; // 524,288

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
	if (order.type == OrderType::Market) {
		if (order.side == Side::Buy) {
			while (order.quantity > 0) {
				if (best_ask == MAX_PRICE) {
					break;
				}

				OrderIndex index = sell_orders[best_ask % ORDER_POOL_SIZE].head;
				while (index == INVALID_INDEX) {
					best_ask++;
					index = sell_orders[best_ask % ORDER_POOL_SIZE].head;
				}
				Order& match = mem_pool[index];

				Quantity min_quantity = std::min(match.quantity, order.quantity);
				match.quantity -= min_quantity;
				order.quantity -= min_quantity;

				if (match.quantity == 0) {
					order_map[match.id] = INVALID_INDEX;

					sell_orders[best_ask % ORDER_POOL_SIZE].head = match.next;

					OrderIndex prev_free = free_list_head;
					free_list_head = index;
					mem_pool[free_list_head].next = prev_free;

				}
			}
		}
		else {
			while (order.quantity > 0) {
				if (best_bid == MIN_PRICE) {
					break;
				}

				OrderIndex index = buy_orders[best_bid % ORDER_POOL_SIZE].head;
				while (index == INVALID_INDEX) {
					best_bid--;
					index = buy_orders[best_bid % ORDER_POOL_SIZE].head;
				}
				Order& match = mem_pool[index];

				Quantity min_quantity = std::min(match.quantity, order.quantity);
				match.quantity -= min_quantity;
				order.quantity -= min_quantity;

				if (match.quantity == 0) {
					order_map[match.id] = INVALID_INDEX;

					buy_orders[best_bid % ORDER_POOL_SIZE].head = match.next;

					OrderIndex prev_free = free_list_head;
					free_list_head = index;
					mem_pool[free_list_head].next = prev_free;
					
				}
			}
		}
	}
	
	if (order.side == Side::Buy) {

	}
}

void LOB::cancel(uint64_t order_id) {

}
