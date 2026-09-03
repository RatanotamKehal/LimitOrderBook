#include "lob.h"

std::vector<Trade> lob::LOB::add(Order order) {
	std::vector<Trade> trades{};

	if (order.order_type == OrderType::Market) {
		return match_order(order);
	}
	if (order.side == Side::Buy) {
		if (!m_sell_orders.empty() && order.price >= m_sell_orders.begin()->first) {
			trades = match_order(order);
		}
		if (order.quantity > 0) {
			m_buy_orders[order.price].push_back(order);
			m_orderID_map[order.order_id] = order;
		}

	}
	else {
		if (!m_buy_orders.empty() && order.price <= m_buy_orders.begin()->first) {
			trades = match_order(order);
		}
		if (order.quantity > 0) {
			m_sell_orders[order.price].push_back(order);
			m_orderID_map[order.order_id] = order;
		}
	}
	return trades;
}

lob::LOB::cancel