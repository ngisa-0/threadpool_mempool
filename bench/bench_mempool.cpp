#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "MemoryPool.h"

namespace {

    constexpr std::size_t kBlockSize = 64;      // 每块 64 字节，模拟任务节点
    constexpr std::size_t kOps = 10'000'000;  // 1000 万次操作
    constexpr std::size_t kPoolBlocks = 1024;   // 池子块数

    // 用一个全局 volatile，防止编译器把循环优化掉
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
    std::cout << "=== new/delete vs MemoryPool ===\n";
    std::cout << "ops = " << kOps
        << ", blockSize = " << kBlockSize
        << ", poolBlocks = " << kPoolBlocks << "\n\n";

    // ---- 基线：new / delete ----
    {
        long long ms = measure_ms([] {
            for (std::size_t i = 0; i < kOps; ++i) {
                char* p = new char[kBlockSize];
                p[0] = static_cast<char>(i);
                g_sink = p[0];
                delete[] p;
            }
            });
        std::cout << "new/delete : " << ms << " ms\n";
    }

    // ---- 对比：MemoryPool ----
    {
        MemoryPool pool(kBlockSize, kPoolBlocks);

        long long ms = measure_ms([&pool] {
            for (std::size_t i = 0; i < kOps; ++i) {
                void* p = pool.allocate();
                static_cast<char*>(p)[0] = static_cast<char>(i);
                g_sink = static_cast<char*>(p)[0];
                pool.deallocate(p);
            }
            });
        std::cout << "MemoryPool : " << ms << " ms\n";
    }

    return 0;
}