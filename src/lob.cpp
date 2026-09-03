#include "lob.h"
#include <chrono>
#include <cassert>

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

std::vector<Trade> lob::LOB::match_order(Order& taker_order) {
    std::vector<Trade> trades;

    if (taker_order.side == Side::Buy) {
        while (taker_order.quantity > 0 && !m_sell_orders.empty()) {
            auto best_ask_iterator = m_sell_orders.begin();
            Price best_price = best_ask_iterator->first;

            if (taker_order.order_type == OrderType::Limit && taker_order.price < best_price) {
                break;
            }

            std::deque<Order>& order_queue = best_ask_iterator->second;
            Order& maker_order = order_queue.front();

            Quantity trade_quantity = std::min(taker_order.quantity, maker_order.quantity);
            Trade trade = {maker_order.order_id, taker_order.order_id, best_price, trade_quantity,
            std::chrono::system_clock::now().time_since_epoch().count() };
            trades.push_back(trade);
            
            taker_order.quantity -= trade_quantity;
            maker_order.quantity -= trade_quantity;

            if (maker_order.quantity == 0) {
                m_orderID_map.erase(maker_order.order_id);
                order_queue.pop_front();
            }
            if (order_queue.empty()) {
                m_sell_orders.erase(best_ask_iterator);
            }
        }
    }
    else {
        while (taker_order.quantity > 0 && !m_buy_orders.empty()) {
            auto best_bid_iterator = m_buy_orders.begin();
			Price best_price = best_bid_iterator->first;

            if (taker_order.order_type == OrderType::Limit && taker_order.price > best_price) {
                break;
            }

			std::deque<Order>& order_queue = best_bid_iterator->second;
			Order& maker_order = order_queue.front();

            Quantity trade_quantity = std::min(taker_order.quantity, maker_order.quantity);
            Trade trade = {maker_order.order_id, taker_order.order_id, best_price, trade_quantity,
                std::chrono::system_clock::now().time_since_epoch().count() };
            trades.push_back(trade);
            
            taker_order.quantity -= trade_quantity;
            maker_order.quantity -= trade_quantity;

            if (maker_order.quantity == 0) {
                m_orderID_map.erase(maker_order.order_id);
                order_queue.pop_front();
            }
            if (order_queue.empty()) {
                m_buy_orders.erase(best_bid_iterator);
			}
		}
    }

    return trades;
}

bool lob::LOB::cancel(const CancelRequest& request) {
    auto it = m_orderID_map.find(request.id);

    if (it == m_orderID_map.end()) { return false; }
    
    Order& order = it->second;

    std::deque<Order>* order_queue_ptr = nullptr;
    if (order.side == Side::Buy) {
       order_queue_ptr = &m_buy_orders[order.price];
    }
    else {
        order_queue_ptr = &m_sell_orders[order.price];
    }
    std::deque<Order>& order_queue = *order_queue_ptr;

    for (auto i = order_queue.begin(); i != order_queue.end(); i++) {
        if (i->order_id == order.order_id) {
            order_queue.erase(i);
            if (order_queue.empty()) {
                if (order.side == Side::Buy) {
                    m_buy_orders.erase(order.price);
                }
                else {
                    m_sell_orders.erase(order.price);
                }
            }

            m_orderID_map.erase(it);

            return true;
        }
    }
    
    assert(false && "Order found in ID map but missing from the price queue.");
    return false;
}