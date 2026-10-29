#pragma once

#include <cstddef>

class ThreadLocalPool {
public:
    ThreadLocalPool(std::size_t blockSize, std::size_t blocksPerThread);
    ~ThreadLocalPool() = default;

    ThreadLocalPool(const ThreadLocalPool&) = delete;
    ThreadLocalPool& operator=(const ThreadLocalPool&) = delete;

    void* allocate();
    void  deallocate(void* ptr);

    std::size_t blockSize() const { return blockSize_; }
    std::size_t blocksPerThread() const { return blocksPerThread_; }

private:
    std::size_t blockSize_;
    std::size_t blocksPerThread_;
};