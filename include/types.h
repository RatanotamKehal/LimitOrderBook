#pragma once

namespace lob {

	using Price = uint64_t;
	using Quantity = uint64_t;
	using OrderID = uint64_t;
	using Timestamp = uint64_t;

	constexpr uint64_t PRICE_MULTIPLIER = 10000; // 10,000

	enum class Side : uint8_t {
		Buy = 0,
		Sell = 1,
	};

	enum class OrderType : uint8_t {
		Limit = 0,
		Market = 1,
	};

	struct Order {
		OrderID order_id;

		Price price;
		Quantity quantity;
		Timestamp timestamp; // timestamp in ns

		OrderType order_type;
		Side side;
	};

	struct Trade {
		OrderID maker_id;
		OrderID taker_id;
		Price price;
		Quantity quantity;
		Timestamp timestamp;
	};

	struct CancelRequest {
		OrderID id;
	};

	static_assert(sizeof(Order) == 40, "Order struct size is not 40 bytes");
}