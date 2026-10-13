#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

#include "ThreadPool.h"

int add(int a, int b) { return a + b; }

int main() {
    std::cout << "threadpool + mempool project start\n";

    ThreadPool pool(4);

    // 提交 8 个任务，每个返回一个 int
    std::vector<std::future<int>> results;
    for (int i = 0; i < 8; ++i) {
        results.push_back(pool.submit(add, i, i * 10));
    }
	//std::cout << results[3].get() << std::endl;

    for (int i = 0; i < 8; ++i) {
        std::cout << i << " + " << i * 10 << " = " << results[i].get() << "\n";
    }

    return 0;
}