#pragma once

alias Price = uint64_t;
alias Quantity = uint64_t;

enum class Side {
	Buy,
	Sell,
};

enum class OrderType {
	Limit,
	Market,
};

struct Order {
	uint64_t order_id;
	Price price;
	Quantity quantity;
	Side side;
	uint64_t arrival_time; // timestamp in ms
};