#pragma once

using Price = uint64_t;
using Quantity = uint64_t;

enum class Side : uint64_t {
	Buy,
	Sell,
};

enum class OrderType : uint64_t {
	Limit,
	Market,
};

struct Order {
	uint64_t order_id;
	Price price;
	Quantity quantity;
	uint64_t arrival_time; // timestamp in ms
	Side side;
	OrderType order_type;
};