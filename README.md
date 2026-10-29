# 高性能线程池与内存池整合组件 (ThreadPool + ThreadCachePool)

本项目基于 **C++17**，从零设计并实现了一个高并发场景下的线程池，并为其量身定制了高性能的线程本地缓存内存池。解决了传统 `new/delete` 频繁分配释放带来的开销，以及全局锁竞争导致的性能瓶颈。

## 📖 项目架构

项目由三个核心模块构成：

1. **ThreadPool（线程池）**：基于 `std::thread` 和 `std::condition_variable`。支持提交任意参数、任意返回值的任务，返回 `std::future` 获取异步结果。实现了任务队列满载时的“背压（Back Pressure）”阻塞机制。
2. **MemoryPool（定长内存池）**：使用空闲链表（Free List）管理内存，16 字节地址对齐。针对 `std::function` 等非平凡类型，使用 `placement new` 和显式析构函数精准控制生命周期。
3. **ThreadCachePool（线程本地缓存池）**：利用 `thread_local` 为每个线程建立独立的空闲链表，实现**无锁**的本地分配与释放。通过记录内存块地址边界（Start/End），安全处理跨线程内存回收，杜绝野指针。

## 🚀 核心优化与性能数据

在 8 线程、块大小 64 字节、单线程 100 万次操作的 Benchmark 压测下：

| 测试场景         | 原生 `new/delete` | 全局锁内存池 | **ThreadCachePool** | 提升倍数         |
|:------------ |:--------------- |:------ |:------------------- |:------------ |
| **同线程分配/释放** | 172 ms          | 327 ms | **36 ms**           | **约 9.08 倍** |
| **跨线程分配/释放** | -               | 10 ms  | 10 ms               | 持平（受限于全局锁）   |

*注：跨线程场景（主线程分配、子线程释放）必须加锁回收，因此与全局锁性能持平，但通过地址边界判断，彻底避免了内存越界与 Use-After-Free 崩溃。*

## 🛠️ 构建与运行

### 依赖要求

- C++17 编译器（MSVC 2019+ / GCC 9+ / Clang 10+）
- CMake 3.16+

### 编译步骤

```bash

# 1. 生成构建目录（以 Release 模式为准）

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -A x64

# 2. 编译

cmake --build build --config Release

# 3. 运行示例程序

./build/Release/tpmp.exe
