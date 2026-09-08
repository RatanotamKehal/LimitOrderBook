#include "lob.h"
#include <limits>
#include <algorithm>
#include <bit>

LOB::LOB() {
	buy_orders.resize(ORDER_POOL_SIZE, { INVALID_INDEX, INVALID_INDEX });
	sell_orders.resize(ORDER_POOL_SIZE, { INVALID_INDEX, INVALID_INDEX });
	buy_bitvector.resize(ORDER_POOL_SIZE >> 6, 0);
	sell_bitvector.resize(ORDER_POOL_SIZE >> 6, 0);
	mem_pool.resize(MEM_POOL_SIZE);

	free_list_head = 0;

	for (OrderIndex i = 0; i + 1 < MEM_POOL_SIZE; i++) {
		mem_pool[i].next = i + 1;
		mem_pool[i].generation = 0;
	}
	mem_pool[MEM_POOL_SIZE - 1].next = INVALID_INDEX;
	mem_pool[MEM_POOL_SIZE - 1].generation = 0;

	best_bid = MIN_PRICE;
	best_ask = MAX_PRICE;

	has_bids = false;
	has_asks = false;
}

inline void LOB::freeOrder(OrderIndex index) {
	Order& order = mem_pool[index];
	++order.generation;
	order.prev = INVALID_INDEX;
	order.next = free_list_head;
	free_list_head = index;
}


inline void LOB::setSellBit(Price price) {
	const uint64_t block_index = price >> 6;
	const uint64_t bit_position = price & 63;
	sell_bitvector[block_index] |= (1ULL << bit_position);
}

inline void LOB::setBuyBit(Price price) {
	const uint64_t bit_index = price >> 6;
	const uint64_t bit_location = price & 63;
	buy_bitvector[bit_index] |= (1ULL << bit_location);
}

inline void LOB::clearSellBit(Price price) {
	const uint64_t bit_index = price >> 6;
	const uint64_t bit_location = price & 63;
	sell_bitvector[bit_index] &= ~(1ULL << bit_location);
}

inline void LOB::clearBuyBit(Price price) {
	const uint64_t bit_index = price >> 6;
	const uint64_t bit_location = price & 63;
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
			if (limit_price < best_ask) { break; }

			index = sell_orders[best_ask].head;
		}

		Order& match = mem_pool[index];
		Quantity min_quantity = std::min(match.quantity, quantity);
		match.quantity -= min_quantity;
		quantity -= min_quantity;

		if (match.quantity == 0) {
			sell_orders[best_ask].head = match.next;
			if (match.next == INVALID_INDEX) {
				sell_orders[best_ask].tail = INVALID_INDEX;
				clearSellBit(best_ask);
			}
			else {
				mem_pool[match.next].prev = INVALID_INDEX;
			}

			freeOrder(index);
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
			if (limit_price > best_bid) { break; }

			index = buy_orders[best_bid].head;
		}

		Order& match = mem_pool[index];
		Quantity min_quantity = std::min(match.quantity, quantity);
		match.quantity -= min_quantity;
		quantity -= min_quantity;

		if (match.quantity == 0) {
			buy_orders[best_bid].head = match.next;
			if (match.next == INVALID_INDEX) {
				buy_orders[best_bid].tail = INVALID_INDEX;
				clearBuyBit(best_bid);
			}
			else {
				mem_pool[match.next].prev = INVALID_INDEX;
			}

			freeOrder(index);
		}
	}
	return quantity;
}

inline AddResult LOB::addRemainingToList(Order& order) {
	if (free_list_head == INVALID_INDEX) {
		// Out of Memory log
		return { order.quantity, INVALID_ORDER_ID };
	};

	OrderIndex allocated_index = free_list_head;
	free_list_head = mem_pool[free_list_head].next;

	Generation curr_gen = ++mem_pool[allocated_index].generation;

	if (order.side == Side::Buy) {
		if (!has_bids || order.price > best_bid) {
			best_bid = order.price;
			has_bids = true;
		}
	}
	else {
		if (!has_asks || order.price < best_ask) {
			best_ask = order.price;
			has_asks = true;
		}
	}

	std::vector<PriceLevel>& orders = (order.side == Side::Buy) ? buy_orders : sell_orders;
	PriceLevel& level = orders[order.price];

	if (level.head == INVALID_INDEX) {
		(order.side == Side::Buy) ? setBuyBit(order.price) : setSellBit(order.price);

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
	order.generation = curr_gen;
	mem_pool[allocated_index] = order;
	
	OrderID id = makeOrderID(allocated_index, curr_gen);

	return { 0, id };
}


AddResult LOB::add(Order& order) {
	if (order.price > MAX_PRICE) {
		// Log Price Error and reject the order
		return { order.quantity, INVALID_ORDER_ID };
	}
	if (order.side == Side::Buy) {
		order.quantity = matchAgainstAsks(order.quantity, (order.type == OrderType::Market) ? MAX_PRICE : order.price);
	}
	else {
		order.quantity = matchAgainstBids(order.quantity, (order.type == OrderType::Market) ? MIN_PRICE : order.price);
	}

	if (order.quantity == 0 || order.type == OrderType::Market) {
		return { order.quantity, INVALID_ORDER_ID };
	}

	return addRemainingToList(order);
}



bool LOB::cancel(OrderID order_id) {
	OrderIndex index = getOrderIndex(order_id);
	Generation generation = getGeneration(order_id);

	if (index >= mem_pool.size()) { return false; }

	Order& order = mem_pool[index];
	if (order.generation != generation) { return false; }

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

	freeOrder(index);
	return true;
}
