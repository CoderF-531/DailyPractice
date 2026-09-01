#define _CRT_SECURE_NO_WARNINGS

#include "ThreadPool.hpp"
using namespace ThreadPoolModel;

void TestThreadPoolSort() {
	std::list<int> nlist = { 6,1,0,5,2,9,11 };

	auto sortlist = pool_thread_quick_sort<int>(nlist);

	for (auto& value : sortlist) {
		std::cout << value << " ";
	}

	std::cout << std::endl;
}



int main()
{
	TestThreadPoolSort();
	return 0;
}
