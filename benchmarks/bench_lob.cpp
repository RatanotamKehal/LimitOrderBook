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


	for (int i = 0; i < COMMAND_COUNT; i++) {
		int roll = probability_distrib(gen);
		Price price = price_distrib(gen);
		Quantity qty = qty_distrib(gen);
		bool side = side_distrib(gen);



		if (roll <= 70) {
			commands.push_back(Command{ false, Order{ } });
		}
		commands.push_back(Command{});
	}

	for (auto _ : state) {

	}
}
