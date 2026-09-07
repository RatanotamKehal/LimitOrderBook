#include "lob.h"
#include <limits>
#include <algorithm>
#include <cmath>

constexpr OrderIndex INVALID_INDEX = std::numeric_limits<OrderIndex>::max();
constexpr Price MAX_PRICE = std::numeric_limits<Price>::max();
constexpr Price MIN_PRICE = 0;

constexpr uint64_t MEM_POOL_SIZE{ 1 << 20 }; // 1,048,576
constexpr uint64_t ORDER_POOL_SIZE{ 1 << 14 }; // 16,384
constexpr uint64_t SPARSE_THRESHOLD{ 1 << 13 }; // 8,192

Price median_price;
Quantity total_orders;

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

Quantity LOB::matchAgainstAsks(Quantity quantity, Price limit_price) {
	while (quantity > 0 && limit_price >= best_ask) {
		OrderIndex index = sell_orders[best_ask % ORDER_POOL_SIZE].head;

		while (index == INVALID_INDEX) {
			if (best_ask == MAX_PRICE) return quantity;
			best_ask++;
			index = sell_orders[best_ask % ORDER_POOL_SIZE].head;
		}

		Order& match = mem_pool[index];
		Quantity min_quantity = std::min(match.quantity, quantity);
		match.quantity -= min_quantity;
		quantity -= min_quantity;

		if (match.quantity == 0) {
			order_map[match.id] = INVALID_INDEX;

			sell_orders[best_ask % ORDER_POOL_SIZE].head = match.next;
			if (match.next == INVALID_INDEX) {
				sell_orders[best_ask % ORDER_POOL_SIZE].tail = INVALID_INDEX;
			}
			else {
				mem_pool[match.next].prev = INVALID_INDEX;
			}

			OrderIndex prev_free = free_list_head;
			free_list_head = index;
			mem_pool[free_list_head].next = prev_free;
		}
	}
	return quantity;
}

Quantity LOB::matchAgainstBids(Quantity quantity, Price limit_price) {
	while (quantity > 0 && limit_price <= best_bid) {
		OrderIndex index = buy_orders[best_bid % ORDER_POOL_SIZE].head;

		while (index == INVALID_INDEX) {
			if (best_bid == MIN_PRICE) return quantity;
			best_bid--;
			index = buy_orders[best_bid % ORDER_POOL_SIZE].head;
		}

		Order& match = mem_pool[index];
		Quantity min_quantity = std::min(match.quantity, quantity);
		match.quantity -= min_quantity;
		quantity -= min_quantity;

		if (match.quantity == 0) {
			order_map[match.id] = INVALID_INDEX;

			buy_orders[best_bid % ORDER_POOL_SIZE].head = match.next;
			if (match.next == INVALID_INDEX) {
				buy_orders[best_bid % ORDER_POOL_SIZE].tail = INVALID_INDEX;
			}
			else {
				mem_pool[match.next].prev = INVALID_INDEX;
			}

			OrderIndex prev_free = free_list_head;
			free_list_head = index;
			mem_pool[free_list_head].next = prev_free;
		}
	}
	return quantity;
}

void LOB::addRemainingToList(Order& order) {
	if (order.quantity > 0 && order.type == OrderType::Limit) {
		total_orders++;
		median_price = ((median_price * total_orders) + order.price) / total_orders;

		if (free_list_head == INVALID_INDEX) {
			// Log OOM error and reject the order
			return;
		}

		OrderIndex allocated_index = free_list_head;
		free_list_head = mem_pool[free_list_head].next;

		order_map[order.id] = allocated_index;

		if (order.side == Side::Buy) {
			if (order.price > best_bid) {
				best_bid = order.price;
			}
		}
		else {
			if (order.price < best_ask) {
				best_ask = order.price;
			}
		}

		std::vector<PriceLevel>& orders = (order.side == Side::Buy) ? buy_orders : sell_orders;
		PriceLevel& level = orders[order.price % ORDER_POOL_SIZE];

		if (level.head == INVALID_INDEX) {
			level.head = allocated_index;
			level.tail = allocated_index;
			order.prev = INVALID_INDEX;
		}
		else {
			mem_pool[level.tail].next = allocated_index;
			order.prev = level.tail;
			level.tail = allocated_index;
		}

		order.next = INVALID_INDEX;
		mem_pool[allocated_index] = order;
	}
}


void LOB::add(Order& order) {
	Price difference = std::max(order.price, median_price) - std::min(order.price, median_price);
	if (difference < SPARSE_THRESHOLD) {
		if (order.side == Side::Buy) {
			order.quantity = matchAgainstAsks(order.quantity, (order.type == OrderType::Market) ? MAX_PRICE : order.price);
		}
		else {
			order.quantity = matchAgainstBids(order.quantity, (order.type == OrderType::Market) ? MIN_PRICE : order.price);
		}
		addRemainingToList(order);
	}
	else {

	}
}



void LOB::cancel(uint64_t order_id) {

}
