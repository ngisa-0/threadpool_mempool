#include "ThreadCachePool.h"
#include <algorithm>

namespace {
    constexpr std::size_t kAlign = 16;
    std::size_t alignUp(std::size_t n) {
        return (n + kAlign - 1) / kAlign * kAlign;
    }

    struct ThreadCache {
        char* memoryStart = nullptr;
        char* memoryEnd = nullptr;
        void* freeList = nullptr;
        // 注意：这里不再有 delete[] memoryStart
    };
    thread_local ThreadCache t_cache;
}

ThreadCachePool::ThreadCachePool(std::size_t blockSize, std::size_t blocksPerBatch)
    : blockSize_(alignUp(std::max(blockSize, sizeof(void*)))),
    blocksPerBatch_(blocksPerBatch) {
}

ThreadCachePool::~ThreadCachePool() {
    // 统一回收所有大块
    for (char* block : blocks_) {
        delete[] block;
    }
}

char* ThreadCachePool::allocate_new_block() {
    char* memory = new char[blockSize_ * blocksPerBatch_];
    {
        std::lock_guard<std::mutex> lock(blocksMutex_);
        blocks_.push_back(memory);
    }
    return memory;
}

void* ThreadCachePool::allocate() {
    if (t_cache.freeList != nullptr) {
        void* p = t_cache.freeList;
        t_cache.freeList = *reinterpret_cast<void**>(p);
        return p;
    }

    // 尝试从全局池拿
    {
        std::lock_guard<std::mutex> lock(globalMutex_);
        std::size_t count = 0;
        while (globalFreeList_ != nullptr && count < blocksPerBatch_) {
            void* p = globalFreeList_;
            globalFreeList_ = *reinterpret_cast<void**>(p);
            *reinterpret_cast<void**>(p) = t_cache.freeList;
            t_cache.freeList = p;
            ++count;
        }
    }

    // 全局也没有，申请新的大块
    if (t_cache.freeList == nullptr) {
        t_cache.memoryStart = allocate_new_block();
        t_cache.memoryEnd = t_cache.memoryStart + blockSize_ * blocksPerBatch_;
        for (std::size_t i = 0; i < blocksPerBatch_; ++i) {
            void* p = t_cache.memoryStart + i * blockSize_;
            *reinterpret_cast<void**>(p) = t_cache.freeList;
            t_cache.freeList = p;
        }
    }

    void* p = t_cache.freeList;
    t_cache.freeList = *reinterpret_cast<void**>(p);
    return p;
}

void ThreadCachePool::deallocate(void* ptr) {
    if (ptr == nullptr) return;

    char* p = static_cast<char*>(ptr);
    if (t_cache.memoryStart && p >= t_cache.memoryStart && p < t_cache.memoryEnd) {
        *reinterpret_cast<void**>(ptr) = t_cache.freeList;
        t_cache.freeList = ptr;
    }
    else {
        std::lock_guard<std::mutex> lock(globalMutex_);
        *reinterpret_cast<void**>(ptr) = globalFreeList_;
        globalFreeList_ = ptr;
    }
}