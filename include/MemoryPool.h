#pragma once

#include <cstddef>
#include <mutex>

class MemoryPool {
public:
    explicit MemoryPool(std::size_t blockSize,
        std::size_t blockCount);

    ~MemoryPool();

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    void* allocate();
    void deallocate(void* ptr);

    std::size_t blockSize() const { return blockSize_; }
    std::size_t blockCount() const { return blockCount_; }

private:
    std::size_t blockSize_;
    std::size_t blockCount_;
    char* memory_;
    void* freeList_;
    std::mutex  mutex_;   // ÐÂÔö
};