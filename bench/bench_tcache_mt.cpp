#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

#include "MemoryPool.h"
#include "ThreadCachePool.h"

namespace {

    constexpr std::size_t kBlockSize = 64;
    constexpr std::size_t kOpsPerThread = 500'000;
    constexpr std::size_t kThreads = 8;
    constexpr std::size_t kPoolBlocks = 1024;

    volatile char g_sink = 0;

    template <typename F>
    long long measure_ms(F&& f) {
        auto start = std::chrono::steady_clock::now();
        f();
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    // ================= 场景 1：同线程分配释放 =================
    void run_same_thread() {
        std::cout << "=== 场景 1：同线程分配释放 ===\n";

        // MemoryPool
        {
            MemoryPool pool(kBlockSize, kPoolBlocks);
            long long ms = measure_ms([&pool] {
                std::vector<std::thread> ts;
                for (std::size_t t = 0; t < kThreads; ++t) {
                    ts.emplace_back([&pool] {
                        for (std::size_t i = 0; i < kOpsPerThread; ++i) {
                            void* p = pool.allocate();
                            static_cast<char*>(p)[0] = static_cast<char>(i);
                            g_sink = static_cast<char*>(p)[0];
                            pool.deallocate(p);
                        }
                        });
                }
                for (auto& t : ts) t.join();
                });
            std::cout << "MemoryPool     : " << ms << " ms\n";
        }

        // ThreadCachePool
        {
            ThreadCachePool pool(kBlockSize, 32); // 一次批发 32 个
            long long ms = measure_ms([&pool] {
                std::vector<std::thread> ts;
                for (std::size_t t = 0; t < kThreads; ++t) {
                    ts.emplace_back([&pool] {
                        for (std::size_t i = 0; i < kOpsPerThread; ++i) {
                            void* p = pool.allocate();
                            static_cast<char*>(p)[0] = static_cast<char>(i);
                            g_sink = static_cast<char*>(p)[0];
                            pool.deallocate(p);
                        }
                        });
                }
                for (auto& t : ts) t.join();
                });
            std::cout << "ThreadCachePool: " << ms << " ms\n\n";
        }
    }

    // ================= 场景 2：跨线程分配释放 =================
    void run_cross_thread() {
        std::cout << "=== 场景 2：跨线程分配释放 (主线程分，子线程还) ===\n";
        constexpr std::size_t kTotalOps = 100'000;

        // MemoryPool
        {
            MemoryPool pool(kBlockSize, kTotalOps); // 池子够大，一次性分配
            std::vector<void*> ptrs(kTotalOps);

            long long ms = measure_ms([&] {
                // 主线程分配
                for (std::size_t i = 0; i < kTotalOps; ++i) {
                    ptrs[i] = pool.allocate();
                }
                // 子线程释放
                std::atomic<std::size_t> idx{ 0 };
                std::vector<std::thread> ts;
                for (std::size_t t = 0; t < kThreads; ++t) {
                    ts.emplace_back([&] {
                        while (true) {
                            std::size_t i = idx.fetch_add(1);
                            if (i >= kTotalOps) break;
                            pool.deallocate(ptrs[i]);
                        }
                        });
                }
                for (auto& t : ts) t.join();
                });
            std::cout << "MemoryPool     : " << ms << " ms\n";
        }

        // ThreadCachePool
        {
            ThreadCachePool pool(kBlockSize, 32);
            std::vector<void*> ptrs(kTotalOps);

            long long ms = measure_ms([&] {
                // 主线程分配（本地无锁）
                for (std::size_t i = 0; i < kTotalOps; ++i) {
                    ptrs[i] = pool.allocate();
                }
                // 子线程释放（跨线程，走全局锁）
                std::atomic<std::size_t> idx{ 0 };
                std::vector<std::thread> ts;
                for (std::size_t t = 0; t < kThreads; ++t) {
                    ts.emplace_back([&] {
                        while (true) {
                            std::size_t i = idx.fetch_add(1);
                            if (i >= kTotalOps) break;
                            pool.deallocate(ptrs[i]);
                        }
                        });
                }
                for (auto& t : ts) t.join();
                });
            std::cout << "ThreadCachePool: " << ms << " ms\n\n";
        }
    }

} // namespace

int main() {
    std::cout << "=== ThreadCachePool Benchmark ===\n\n";
    run_same_thread();
    run_cross_thread();
    return 0;
}