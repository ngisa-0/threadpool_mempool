#include "MemoryPool.h"

#include <algorithm>
#include <cstdint>
#include <new>

namespace {
    constexpr std::size_t kAlign = 16;

    std::size_t alignUp(std::size_t n) {
        return (n + kAlign - 1) / kAlign * kAlign;
    }
}

MemoryPool::MemoryPool(std::size_t blockSize, std::size_t blockCount)
    : blockSize_(alignUp(std::max(blockSize, sizeof(void*)))),
    blockCount_(blockCount),
    memory_(nullptr),
    freeList_(nullptr),
    freeCount_(0) {

    if (blockCount_ == 0) {
        return;
    }

    memory_ = new char[blockSize_ * blockCount_];
    freeCount_ = blockCount_;

    for (std::size_t i = 0; i < blockCount_; ++i) {
        void* block = memory_ + i * blockSize_;
        *reinterpret_cast<void**>(block) = freeList_;
        freeList_ = block;
    }
}

MemoryPool::~MemoryPool() {
    delete[] memory_;
    memory_ = nullptr;
    freeList_ = nullptr;
    freeCount_ = 0;
}

void* MemoryPool::allocate() {
    std::unique_lock<std::mutex> lock(mutex_);

    // 池满时睡在这里，直到：
    //   1. 有块被归还（freeList_ != nullptr），或
    //   2. 池子被关闭（stop_ == true）
    cv_.wait(lock, [this]() {
        return stop_ || freeList_ != nullptr;
        });

    if (freeList_ == nullptr) {
        // 走到这里说明 stop_ == true 且池子空，关闭了
        return nullptr;
    }

    void* block = freeList_;
    freeList_ = *reinterpret_cast<void**>(block);
    --freeCount_;
    return block;
}

void MemoryPool::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        *reinterpret_cast<void**>(ptr) = freeList_;
        freeList_ = ptr;
        ++freeCount_;
    }
    cv_.notify_one();   // 锁外唤醒一个等块的线程
}

void MemoryPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
}

std::size_t MemoryPool::freeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return freeCount_;
}

std::size_t MemoryPool::usedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return blockCount_ - freeCount_;
}