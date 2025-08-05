#include <benchmark/benchmark.h>
#include <thread>
#include "atomic-queue.h" // 假设 AtomicQueue 的实现位于此文件中

using namespace tp;

// 单线程 push 测试
static void BM_SingleThreadPush(benchmark::State &state) {
    AtomicQueue<int> q;
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            q.push(i);
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// 单线程 pop 测试
static void BM_SingleThreadPop(benchmark::State &state) {
    AtomicQueue<int> q;
    for (int i = 0; i < state.range(0); ++i) {
        q.push(i);
    }
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            q.pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// 多线程 push 测试
static void BM_MultiThreadPush(benchmark::State &state) {
    AtomicQueue<int> q;
    auto num_threads = state.threads();
    for (auto _ : state) {
        int chunk_size = state.range(0) / num_threads;
        std::thread threads[num_threads];
        for (int t = 0; t < num_threads; ++t) {
            threads[t] = std::thread([&q, chunk_size, t]() {
                for (int i = t * chunk_size; i < (t + 1) * chunk_size; ++i) {
                    q.push(i);
                }
            });
        }
        for (int t = 0; t < num_threads; ++t) {
            threads[t].join();
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// 多线程 pop 测试
static void BM_MultiThreadPop(benchmark::State &state) {
    AtomicQueue<int> q;
    auto num_threads = state.threads();
    for (int i = 0; i < state.range(0); ++i) {
        q.push(i);
    }
    for (auto _ : state) {
        int chunk_size = state.range(0) / num_threads;
        std::thread threads[num_threads];
        for (int t = 0; t < num_threads; ++t) {
            threads[t] = std::thread([&q, chunk_size]() {
                for (int i = 0; i < chunk_size; ++i) {
                    q.pop();
                }
            });
        }
        for (int t = 0; t < num_threads; ++t) {
            threads[t].join();
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// 高并发混合 push 和 pop 测试
static void BM_MultiThreadPushPop(benchmark::State &state) {
    AtomicQueue<int> q;
    auto num_threads = state.threads();
    for (auto _ : state) {
        std::thread threads[num_threads];
        for (int t = 0; t < num_threads; ++t) {
            threads[t] = std::thread([&q, t]() {
                if (t % 2 == 0) {
                    // 偶数线程执行 push
                    for (int i = 0; i < 1000; ++i) {
                        q.push(i);
                    }
                } else {
                    // 奇数线程执行 pop
                    for (int i = 0; i < 1000; ++i) {
                        q.pop();
                    }
                }
            });
        }
        for (int t = 0; t < num_threads; ++t) {
            threads[t].join();
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000 * num_threads);
}

// 注册基准测试
BENCHMARK(BM_SingleThreadPush)->Arg(1000)->Arg(10000)->Arg(100000);
BENCHMARK(BM_SingleThreadPop)->Arg(1000)->Arg(10000)->Arg(100000);
BENCHMARK(BM_MultiThreadPush)->Arg(1000)->Arg(10000)->Arg(100000)->Threads(2)->Threads(4)->Threads(8);
BENCHMARK(BM_MultiThreadPop)->Arg(1000)->Arg(10000)->Arg(100000)->Threads(2)->Threads(4)->Threads(8);
BENCHMARK(BM_MultiThreadPushPop)->Threads(2)->Threads(4)->Threads(8);

// 启动基准测试
BENCHMARK_MAIN();