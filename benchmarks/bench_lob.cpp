#include "benchmark/benchmark.h"
#include <random>
#include "lob.h"

const int COMMAND_COUNT{ 1000000 }; //1,000,000

using namespace lob;

struct Command {
	bool is_cancel;
	Order order;
};

static void BM_OrderBook(benchmark::State& state) {
	std::mt19937 gen(12);
	std::vector<Command> commands;
	std::vector<OrderID> active_limits_ids;
	
	std::uniform_int_distribution<int> probability_distrib(1, 100);
	std::uniform_int_distribution<int> price_distrib(495, 505);
	std::uniform_int_distribution<int> qty_distrib(1, 100);
	std::uniform_int_distribution<int> side_distrib(0, 1);
	OrderID current_id = 0;

	for (int i = 0; i < COMMAND_COUNT; i++) {
		int roll = probability_distrib(gen);
		Price price = price_distrib(gen);
		Quantity qty = qty_distrib(gen);
		Side side = static_cast<Side>(side_distrib(gen));


		Order order;
		if (roll <= 70) {
			order = {
				current_id++,
				price,
				qty,
				0,
				OrderType::Limit,
				side,
			};
			active_limits_ids.push_back(order.order_id);
			commands.push_back(Command{false, order});
		}
		else if (roll <= 90) {
			std::uniform_int_distribution<int> list_distrib(0, active_limits_ids.size());
			auto random_index = list_distrib(gen);
			commands.push_back(Command{ true, commands.at(random_index).order});
		}
		else {
			order = {
				current_id++,
				0,
				qty,
				0,
				OrderType::Market,
				side,
			};
			active_limits_ids.push_back(order.order_id);
			commands.push_back(Command{ false, order });
		}
	}

	for (auto _ : state) {

	}
}
