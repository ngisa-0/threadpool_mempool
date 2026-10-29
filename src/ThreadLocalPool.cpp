#include "ThreadLocalPool.h"

#include <algorithm>

namespace {

    constexpr std::size_t kAlign = 16;

    std::size_t alignUp(std::size_t n) {
        return (n + kAlign - 1) / kAlign * kAlign;
    }

    struct LocalCache {
        char* memory = nullptr;
        void* freeList = nullptr;
        ~LocalCache() {
            delete[] memory;   // 线程退出时自动回收
        }
    };

    LocalCache& local() {
        thread_local LocalCache cache;
        return cache;
    }

} // namespace

ThreadLocalPool::ThreadLocalPool(std::size_t blockSize,
    std::size_t blocksPerThread)
    : blockSize_(alignUp(std::max(blockSize, sizeof(void*)))),
    blocksPerThread_(blocksPerThread) {
}

void* ThreadLocalPool::allocate() {
    LocalCache& lc = local();

    // 第一次调用：分配一大块，切成链表
    if (lc.memory == nullptr) {
        lc.memory = new char[blockSize_ * blocksPerThread_];
        for (std::size_t i = 0; i < blocksPerThread_; ++i) {
            void* b = lc.memory + i * blockSize_;
            *reinterpret_cast<void**>(b) = lc.freeList;
            lc.freeList = b;
        }
    }

    if (lc.freeList == nullptr) {
        return nullptr;   // 本地块用完
    }

    void* p = lc.freeList;
    lc.freeList = *reinterpret_cast<void**>(p);
    return p;
}

void ThreadLocalPool::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    LocalCache& lc = local();
    *reinterpret_cast<void**>(ptr) = lc.freeList;
    lc.freeList = ptr;
}