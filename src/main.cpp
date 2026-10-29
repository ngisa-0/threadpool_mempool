#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "ThreadPool.h"

std::atomic<int> g_total{ 0 };

int heavy(int id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    g_total += id;
    return id * id;
}

int main() {
    std::cout << "threadpool + mempool project start\n";

    // 线程池 4 线程，但任务池只有 4 块
    ThreadPool pool(4, 1);

    std::vector<std::future<int>> results;
    for (int i = 0; i < 20; ++i) {
        // 池子只有 4 块，submit 会在池满时自动阻塞
        // 工作线程执行完任务、归还节点后，submit 继续
        results.push_back(pool.submit(heavy, i));
    }

    for (int i = 0; i < 20; ++i) {
        std::cout << "heavy(" << i << ") = " << results[i].get() << "\n";
    }

    std::cout << "total = " << g_total << "\n";
    return 0;
}