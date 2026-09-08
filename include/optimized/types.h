#pragma once
#include <cstdint>
namespace optimized {

	using Price = uint32_t;
	using Quantity = uint32_t; // For larger quantities, maybe implement Lot Sizes ex. 1 Quantity = 10,000 Shares
	using OrderIndex = uint32_t;
	using Generation = uint32_t;
	using OrderID = uint64_t;


	enum class Side : uint8_t {
		Buy = 0,
		Sell = 1,
	};

	enum class OrderType : uint8_t {
		Limit = 0,
		Market = 1,
	};

	struct Order {
		uint64_t time; // timestamp in ns
		Generation generation;

		Price price;
		Quantity quantity;

		OrderIndex next;
		OrderIndex prev;

		OrderType type;
		Side side;
	};

	struct PriceLevel {
		OrderIndex head;
		OrderIndex tail;
	};

	struct AddResult {
		Quantity remaining_quantity;
		OrderID order_id;
	};
}
