#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

class ThreadCachePool {
public:
    ThreadCachePool(std::size_t blockSize, std::size_t blocksPerBatch);
    ~ThreadCachePool();

    void* allocate();
    void deallocate(void* ptr);

private:
    char* allocate_new_block(); // 向系统申请一个大块

    std::size_t blockSize_;
    std::size_t blocksPerBatch_;
    void* globalFreeList_ = nullptr;
    std::mutex globalMutex_;

    std::vector<char*> blocks_; // 记录所有申请的大块
    std::mutex blocksMutex_;    // 保护 blocks_ 列表
};