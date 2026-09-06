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

void LOB::marketAdd(Order& order) {
	if (order.side == Side::Buy) {
		while (order.quantity > 0) {

			OrderIndex index = sell_orders[best_ask % ORDER_POOL_SIZE].head;
			while (index == INVALID_INDEX) {
				if (best_ask == MAX_PRICE) {
					break;
				}

				best_ask++;
				index = sell_orders[best_ask % ORDER_POOL_SIZE].head;
			}
			if (index == INVALID_INDEX) { break; }

			Order& match = mem_pool[index];

			Quantity min_quantity = std::min(match.quantity, order.quantity);
			match.quantity -= min_quantity;
			order.quantity -= min_quantity;

			if (match.quantity == 0) {
				order_map[match.id] = INVALID_INDEX;

				sell_orders[best_ask % ORDER_POOL_SIZE].head = match.next;
				if (match.next == INVALID_INDEX) {
					sell_orders[best_ask % ORDER_POOL_SIZE].tail = INVALID_INDEX;
				}

				OrderIndex prev_free = free_list_head;
				free_list_head = index;
				mem_pool[free_list_head].next = prev_free;

			}
		}
	}
	else {
		while (order.quantity > 0) {

			OrderIndex index = buy_orders[best_bid % ORDER_POOL_SIZE].head;
			while (index == INVALID_INDEX) {
				if (best_bid == MIN_PRICE) {
					break;
				}

				best_bid--;
				index = buy_orders[best_bid % ORDER_POOL_SIZE].head;
			}
			if (index == INVALID_INDEX) { break; }

			Order& match = mem_pool[index];

			Quantity min_quantity = std::min(match.quantity, order.quantity);
			match.quantity -= min_quantity;
			order.quantity -= min_quantity;

			if (match.quantity == 0) {
				order_map[match.id] = INVALID_INDEX;

				buy_orders[best_bid % ORDER_POOL_SIZE].head = match.next;
				if (match.next == INVALID_INDEX) {
					buy_orders[best_bid % ORDER_POOL_SIZE].tail = INVALID_INDEX;
				}

				OrderIndex prev_free = free_list_head;
				free_list_head = index;
				mem_pool[free_list_head].next = prev_free;

			}
		}
	}
}

void LOB::limitAddBuy(Order& order) {
	while (order.quantity > 0) {
		if (order.price < best_ask) {
			OrderIndex allocated_index = free_list_head;
			OrderIndex next_free = mem_pool[free_list_head].next;
			PriceLevel& buy_level = buy_orders[order.price % ORDER_POOL_SIZE];

			order_map[order.id] = allocated_index;

			if (order.price > best_bid) {
				best_bid = order.price;
			}

			if (buy_level.head == INVALID_INDEX) {
				buy_level.head = allocated_index;
				buy_level.tail = allocated_index;
				order.prev = INVALID_INDEX;
			}
			else {
				mem_pool[buy_level.tail].next = allocated_index;
				order.prev = buy_level.tail;
				buy_level.tail = allocated_index;
			}

			order.next = INVALID_INDEX;
			mem_pool[allocated_index] = order;
			free_list_head = next_free;
			break;
		}
		else {
			while (order.quantity > 0 && order.price >= best_ask) {
				PriceLevel sell_level = sell_orders[best_ask % ORDER_POOL_SIZE];

				Order& match = mem_pool[sell_level.head];

				Quantity quantity = std::min(match.quantity, order.quantity);
				match.quantity -= quantity;
				order.quantity -= quantity;

				if (match.quantity == 0) {
					order_map[match.id] = INVALID_INDEX;

					sell_orders[best_ask % ORDER_POOL_SIZE].head = match.next;
					if (match.next == INVALID_INDEX) {
						sell_orders[best_ask % ORDER_POOL_SIZE].tail = INVALID_INDEX;
						while (sell_orders[best_ask % ORDER_POOL_SIZE].head == INVALID_INDEX) {
							if (best_ask == MAX_PRICE) {
								return;
							}

							best_ask++;
						}
					}

					OrderIndex prev_free = free_list_head;
					free_list_head = sell_level.head;
					mem_pool[free_list_head].next = prev_free;
				}
			}
		}
	}
}

void LOB::limitAddSell(Order& order) {
	while (order.quantity > 0) {
		if (order.price > best_bid) {
			OrderIndex allocated_index = free_list_head;
			OrderIndex next_free = mem_pool[free_list_head].next;
			PriceLevel& sell_level = sell_orders[order.price % ORDER_POOL_SIZE];

			order_map[order.id] = allocated_index;

			if (order.price < best_ask) {
				best_ask = order.price;
			}

			if (sell_level.head == INVALID_INDEX) {
				sell_level.head = allocated_index;
				sell_level.tail = allocated_index;
				order.prev = INVALID_INDEX;
			}
			else {
				mem_pool[sell_level.tail].next = allocated_index;
				order.prev = sell_level.tail;
				sell_level.tail = allocated_index;
			}

			order.next = INVALID_INDEX;
			mem_pool[allocated_index] = order;
			free_list_head = next_free;
			break;
		}
		else {
			while (order.quantity > 0 && order.price <= best_bid) {
				PriceLevel buy_level = buy_orders[best_bid % ORDER_POOL_SIZE];

				Order& match = mem_pool[buy_level.head];

				Quantity quantity = std::min(match.quantity, order.quantity);
				match.quantity -= quantity;
				order.quantity -= quantity;

				if (match.quantity == 0) {
					order_map[match.id] = INVALID_INDEX;

					buy_orders[best_bid % ORDER_POOL_SIZE].head = match.next;
					if (match.next == INVALID_INDEX) {
						sell_orders[best_bid % ORDER_POOL_SIZE].tail = INVALID_INDEX;
						while (sell_orders[best_bid % ORDER_POOL_SIZE].head == INVALID_INDEX) {
							if (best_bid == MIN_PRICE) {
								return;
							}

							best_bid--;
						}
					}

					OrderIndex prev_free = free_list_head;
					free_list_head = buy_level.head;
					mem_pool[free_list_head].next = prev_free;
				}
			}
		}
	}
}

void LOB::add(Order& order) {
	if (order.type == OrderType::Market) {
		marketAdd(order);
	}
	else if (order.type == OrderType::Limit && order.side == Side::Buy) {
		limitAddBuy(order);
	}
	else if (order.type == OrderType::Limit && order.side == Side::Sell) {
		limitAddSell(order);
	}
}

void LOB::cancel(uint64_t order_id) {

}
