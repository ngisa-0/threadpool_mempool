#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>

class MemoryPool {
public:
    explicit MemoryPool(std::size_t blockSize,
        std::size_t blockCount);

    ~MemoryPool();

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // 阻塞版：池满时睡，直到有块被归还；池子关闭后返回 nullptr
    void* allocate();

    void deallocate(void* ptr);

    // 关闭池子，唤醒所有等待者；之后 allocate 不再阻塞，直接返回 nullptr
    void shutdown();

    std::size_t blockSize() const { return blockSize_; }
    std::size_t blockCount() const { return blockCount_; }

    std::size_t freeCount() const;
    std::size_t usedCount() const;

private:
    std::size_t blockSize_;
    std::size_t blockCount_;
    char* memory_;
    void* freeList_;
    std::size_t freeCount_;
    bool        stop_ = false;               // 新增
    mutable std::mutex mutex_;
    std::condition_variable cv_;             // 新增
};