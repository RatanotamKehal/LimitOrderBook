#include "lob.h"
#include <cmath>

const int MEM_POOL_SIZE{ 1000000 };
const int ORDER_POOL_SIZE{ std::log2(MEM_POOL_SIZE) };

LOB::LOB() {
	buy_orders.resize(ORDER_POOL_SIZE);
	sell_orders.resize(ORDER_POOL_SIZE);
	mem_pool.resize(MEM_POOL_SIZE);

	free_list_head = 0;

	for (int i = 0; i < MEM_POOL_SIZE; i++) {
		mem_pool[i].next = i + 1;
		mem_pool[i].prev = i - 1;

		if (i == 0) {
			mem_pool[i].prev = NULL;
		}
		if (i == (MEM_POOL_SIZE - 1)) {
			mem_pool[i].next = NULL;
		}
	}
}

void LOB::add(Order& order) {
	
}

void LOB::cancel(uint64_t order_id) {

}
