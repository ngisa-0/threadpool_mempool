#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <new>

#include "ThreadCachePool.h" // ★ 改成包含新头文件

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount,
        std::size_t taskPoolSize = 1024);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

private:
    // 任务节点，固定大小
    struct TaskNode {
        std::function<void()> fn;
        TaskNode* next;   // 预留：将来可挂成自由链表
    };

    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<TaskNode*>    tasks_;
    ThreadCachePool          taskPool_;  // ★ 替换为 ThreadCachePool
    std::mutex               mutex_;
    std::condition_variable  cv_;
    bool                     stop_ = false;
};

// ============ 模板实现 ============

template <typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
-> std::future<std::invoke_result_t<F, Args...>> {

    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<ReturnType> fut = task->get_future();

    // ★ ThreadCachePool 不会返回 nullptr，直接拿
    TaskNode* node = static_cast<TaskNode*>(taskPool_.allocate());

    // ★ 关键：在 raw 内存上构造 TaskNode
    new (node) TaskNode{};

    node->fn = [task]() { (*task)(); };

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_) {
            // ★ 归还前先析构，否则 std::function 内的资源泄漏
            node->~TaskNode();
            taskPool_.deallocate(node);
            throw std::runtime_error("submit on stopped ThreadPool");
        }
        tasks_.push(node);
    }
    cv_.notify_one();

    return fut;
}