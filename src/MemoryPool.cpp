#include "MemoryPool.h"

#include <algorithm>
#include <cstdint>
#include <new>

MemoryPool::MemoryPool(std::size_t blockSize, std::size_t blockCount)
    : blockSize_(std::max(blockSize, sizeof(void*))),
    blockCount_(blockCount),
    memory_(nullptr),
    freeList_(nullptr) {

    if (blockCount_ == 0) {
        return;
    }

    memory_ = new char[blockSize_ * blockCount_];

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
}

void* MemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (freeList_ == nullptr) {
        return nullptr;
    }
    void* block = freeList_;
    freeList_ = *reinterpret_cast<void**>(block);
    return block;
}

void MemoryPool::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    *reinterpret_cast<void**>(ptr) = freeList_;
    freeList_ = ptr;
}