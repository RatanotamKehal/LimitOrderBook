#pragma once
#include <map>
#include <deque>
#include <vector>
#include <unordered_map>
#include "types.h"

namespace lob {

	class LOB {
	private:
		std::map<Price, std::deque<Order>, std::greater<Price>> m_buy_orders;
		std::map<Price ,std::deque<Order>, std::less<Price>> m_sell_orders;
		std::unordered_map<OrderID, Order> m_orderID_map;
		std::vector<Trade> match_order(Order& taker_order);

	public:
		std::vector<Trade> add(Order order);
		bool cancel(const CancelRequest& request);
		bool has_order(OrderID id) const;
	};
}


