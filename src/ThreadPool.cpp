#include "ThreadPool.h"

ThreadPool::ThreadPool(std::size_t threadCount) {
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
    cv_.notify_all(); //叫醒所有睡着的线程

    for (auto& t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

//void ThreadPool::submit(std::function<void()> task) {
//    {
//        std::lock_guard<std::mutex> lock(mutex_);
//        if (stop_) {
//            return; // 池子已经关了，直接丢
//        }
//        tasks_.push(std::move(task));
//    }
//    cv_.notify_one();//只叫醒一个线程
//}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() {
                return stop_ || !tasks_.empty();
                //（线程池要关闭） OR （有任务要执行）
                });

            if (stop_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}