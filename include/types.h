#pragma once

using Price = uint64_t;
using Quantity = uint64_t;
using OrderIndex = uint32_t;

constexpr uint64_t PRICE_MULTIPLIER = 10000; // 10,000

enum class Side : uint8_t {
	Buy,
	Sell,
};

enum class OrderType : uint8_t {
	Limit,
	Market,
};

struct Order {
	uint64_t order_id;

	Price price;
	Quantity quantity;
	uint64_t arrival_time; // timestamp in ns
	
	OrderIndex next;
	OrderIndex prev;

	OrderType order_type;
	Side side;
};

struct PriceLevel {
	OrderIndex head;
	OrderIndex tail;
};