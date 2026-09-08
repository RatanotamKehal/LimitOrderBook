#include "lob.h"
#include <limits>
#include <algorithm>
#include <bit>

constexpr OrderIndex INVALID_INDEX = std::numeric_limits<OrderIndex>::max();

constexpr uint64_t MEM_POOL_SIZE{ 1 << 20 }; // 1,048,576
constexpr uint64_t ORDER_POOL_SIZE{ 1 << 19 }; // 524,288

constexpr Price MAX_PRICE = ORDER_POOL_SIZE - 1;
constexpr Price MIN_PRICE = 0;

LOB::LOB() {
	buy_orders.resize(ORDER_POOL_SIZE, { INVALID_INDEX, INVALID_INDEX });
	sell_orders.resize(ORDER_POOL_SIZE, { INVALID_INDEX, INVALID_INDEX });
	buy_bitvector.resize(ORDER_POOL_SIZE >> 6, 0);
	sell_bitvector.resize(ORDER_POOL_SIZE >> 6, 0);
	mem_pool.resize(MEM_POOL_SIZE);
	order_map.resize(MEM_POOL_SIZE, INVALID_INDEX);

	free_list_head = 0;

	for (int i = 0; i < MEM_POOL_SIZE; i++) {
		mem_pool[i].next = i + 1;
	}
	mem_pool[MEM_POOL_SIZE - 1].next = INVALID_INDEX;

	best_bid = MIN_PRICE;
	best_ask = MAX_PRICE;

	has_bids = false;
	has_asks = false;
}


inline void LOB::setSellBit(Price price) {
	uint64_t block_index = price >> 6;
	uint64_t bit_position = price & 63;
	sell_bitvector[block_index] |= (1ULL << bit_position);
}

inline void LOB::setBuyBit(Price price) {
	uint64_t bit_index = price >> 6;
	uint64_t bit_location = price & 63;
	buy_bitvector[bit_index] |= (1ULL << bit_location);
}

inline void LOB::clearSellBit(Price price) {
	uint64_t bit_index = price >> 6;
	uint64_t bit_location = price & 63;
	sell_bitvector[bit_index] &= ~(1ULL << bit_location);
}

inline void LOB::clearBuyBit(Price price) {
	uint64_t bit_index = price >> 6;
	uint64_t bit_location = price & 63;
	buy_bitvector[bit_index] &= ~(1ULL << bit_location);
}

inline Quantity LOB::matchAgainstAsks(Quantity quantity, Price limit_price) {
	while (quantity > 0 && has_asks && limit_price >= best_ask) {
		OrderIndex index = sell_orders[best_ask].head;
	
		if (index == INVALID_INDEX) {
			uint64_t block_index = best_ask >> 6;
			uint64_t bit_block = sell_bitvector[block_index];

			while (bit_block == 0) {
				block_index++;

				if (block_index >= sell_bitvector.size()) {
					best_ask = MAX_PRICE;
					has_asks = false;
					return quantity;
				}

				bit_block = sell_bitvector[block_index];
			}

			best_ask = (block_index << 6) + std::countr_zero(bit_block);
			index = sell_orders[best_ask].head;
		}

		Order& match = mem_pool[index];
		Quantity min_quantity = std::min(match.quantity, quantity);
		match.quantity -= min_quantity;
		quantity -= min_quantity;

		if (match.quantity == 0) {
			order_map[match.id] = INVALID_INDEX;

			sell_orders[best_ask].head = match.next;
			if (match.next == INVALID_INDEX) {
				sell_orders[best_ask].tail = INVALID_INDEX;
				clearSellBit(best_ask);
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

inline Quantity LOB::matchAgainstBids(Quantity quantity, Price limit_price) {
	while (quantity > 0 && has_bids && limit_price <= best_bid) {
		OrderIndex index = buy_orders[best_bid].head;

		if (index == INVALID_INDEX) {
			uint64_t block_index = best_bid >> 6;
			uint64_t bit_block = buy_bitvector[block_index];

			while (bit_block == 0) {
				if (block_index == 0) {
					best_bid = MIN_PRICE;
					has_bids = false;
					return quantity;
				}
				block_index--;
				bit_block = buy_bitvector[block_index];
			}

			best_bid = (block_index << 6) + (63 - std::countl_zero(bit_block));
			index = buy_orders[best_bid].head;
		}

		Order& match = mem_pool[index];
		Quantity min_quantity = std::min(match.quantity, quantity);
		match.quantity -= min_quantity;
		quantity -= min_quantity;

		if (match.quantity == 0) {
			order_map[match.id] = INVALID_INDEX;

			buy_orders[best_bid].head = match.next;
			if (match.next == INVALID_INDEX) {
				buy_orders[best_bid].tail = INVALID_INDEX;
				clearBuyBit(best_bid);
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

inline void LOB::addRemainingToList(Order& order) {
	if (order.quantity > 0 && order.type == OrderType::Limit) {

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
		PriceLevel& level = orders[order.price];

		if (level.head == INVALID_INDEX) {
			(order.side == Side::Buy) ? setBuyBit(order.price) : setSellBit(order.price);
			(order.side == Side::Buy) ? has_bids = true : has_asks = true;

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


Quantity LOB::add(Order& order) {
	if (order.price > MAX_PRICE) {
		// Log Price Error and reject the order
		return order.quantity;
	}
	if (order.side == Side::Buy) {
		order.quantity = matchAgainstAsks(order.quantity, (order.type == OrderType::Market) ? MAX_PRICE : order.price);
	}
	else {
		order.quantity = matchAgainstBids(order.quantity, (order.type == OrderType::Market) ? MIN_PRICE : order.price);
	}

	if (order.quantity == 0 || order.type == OrderType::Market) {
		return order.quantity;
	}

	if (free_list_head == INVALID_INDEX) {
		// Log OOM error and return remaining order that wasn't added
		return order.quantity;
	}

	addRemainingToList(order);
	return 0;
}



bool LOB::cancel(uint64_t order_id) {
	if (order_id >= order_map.size()) { return false; }

	OrderIndex index = order_map[order_id];
	if (index == INVALID_INDEX) { return false; }
	
	Order& order = mem_pool[index];

	if (order.next == INVALID_INDEX && order.prev == INVALID_INDEX) {
		if (order.side == Side::Buy) {
			clearBuyBit(order.price);
			buy_orders[order.price].head = INVALID_INDEX;
			buy_orders[order.price].tail = INVALID_INDEX;
		}
		else {
			clearSellBit(order.price);
			sell_orders[order.price].head = INVALID_INDEX;
			sell_orders[order.price].tail = INVALID_INDEX;
		}
	}
	else if (order.next == INVALID_INDEX) {
		if (order.side == Side::Buy) {
			buy_orders[order.price].tail = order.prev;
		}
		else {
			sell_orders[order.price].tail = order.prev;
		}
		mem_pool[order.prev].next = INVALID_INDEX;
	}
	else if (order.prev == INVALID_INDEX) {
		if (order.side == Side::Buy) {
			buy_orders[order.price].head = order.next;
		}
		else {
			sell_orders[order.price].head = order.next;
		}
		mem_pool[order.next].prev = INVALID_INDEX;
	}
	else {
		mem_pool[order.prev].next = order.next;
		mem_pool[order.next].prev = order.prev;
	}

	OrderIndex prev_free = free_list_head;
	free_list_head = index;
	order.prev = INVALID_INDEX;
	order.next = prev_free;

	order_map[order_id] = INVALID_INDEX;
	return true;
}
