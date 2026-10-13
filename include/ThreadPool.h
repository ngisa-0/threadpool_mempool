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

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 简单版：无参数、无返回值的任务
    //void submit(std::function<void()> task);

    // 通用版：任意函数、任意参数、返回 future
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

private:
    void workerLoop();

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mutex_;
    std::condition_variable           cv_;
    bool                              stop_ = false;
};

template <typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
-> std::future<std::invoke_result_t<F, Args...>> {

    using ReturnType = std::invoke_result_t<F, Args...>;

    // 把 f 和 args 打包成 packaged_task
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
	
    //std::bind:把函数和参数绑定成一个可调用对象，返回一个新的可调用对象。
    //std::forward 完美转发:把参数原样转发，保留它是左值还是右值，不强行拷贝。
	//std::packaged_task:把一个可调用对象包装成一个任务，可以异步执行，并且可以获取返回值。
	//std::make_shared:创建一个共享指针，管理动态分配的对象，避免手动释放内存。

    std::future<ReturnType> fut = task->get_future();

    // 转成 std::function<void()> 塞进队列
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_) {
            // 池子已关，扔一个异常给调用者
            throw std::runtime_error("submit on stopped ThreadPool");
        }
        tasks_.push([task]() { (*task)(); });
    }
    cv_.notify_one();

    return fut;
}