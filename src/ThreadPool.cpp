#include "ThreadPool.h"

ThreadPool::ThreadPool(std::size_t threadCount, std::size_t taskPoolSize)
    : taskPool_(sizeof(TaskNode), taskPoolSize) {
    // ★ 这里的 taskPoolSize 语义变了：表示每个线程每次去全局池“批发”多少个块

    if (threadCount == 0) {
        threadCount = 1;
    }
    workers_.reserve(threadCount);

    for (std::size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back([this]() { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    // ★ 删掉了 taskPool_.shutdown(); 因为新池子根本没有这个方法
    cv_.notify_all();

    for (auto& t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::workerLoop() {
    while (true) {
        TaskNode* node = nullptr;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() {
                return stop_ || !tasks_.empty();
                });

            if (stop_ && tasks_.empty()) {
                return;
            }

            node = tasks_.front();
            tasks_.pop();
        }

        node->fn();

        // ★ 显式析构，释放 std::function 内部资源
        node->~TaskNode();

        // 再归还给池子
        taskPool_.deallocate(node);
    }
}