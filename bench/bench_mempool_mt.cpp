#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

#include "MemoryPool.h"

namespace {

    constexpr std::size_t kBlockSize = 64;
    constexpr std::size_t kOpsPerThread = 1'000'000;
    constexpr std::size_t kThreads = 8;
    constexpr std::size_t kPoolBlocks = 4096;

    volatile char g_sink = 0;

    template <typename F>
    long long measure_ms(F&& f) {
        auto start = std::chrono::steady_clock::now();
        f();
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

} // namespace

int main() {
    std::cout << "=== multi-thread: new/delete vs MemoryPool ===\n";
    std::cout << "threads = " << kThreads
        << ", ops/thread = " << kOpsPerThread
        << ", blockSize = " << kBlockSize << "\n\n";

    // ---- A: new/delete ----
    {
        long long ms = measure_ms([] {
            std::vector<std::thread> ts;
            for (std::size_t t = 0; t < kThreads; ++t) {
                ts.emplace_back([] {
                    for (std::size_t i = 0; i < kOpsPerThread; ++i) {
                        char* p = new char[kBlockSize];
                        p[0] = static_cast<char>(i);
                        g_sink = p[0];
                        delete[] p;
                    }
                    });
            }
            for (auto& t : ts) t.join();
            });
        std::cout << "new/delete : " << ms << " ms\n";
    }

    // ---- B: MemoryPool ----
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
        std::cout << "MemoryPool : " << ms << " ms\n";
    }

    return 0;
}