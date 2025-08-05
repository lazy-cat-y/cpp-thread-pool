
#include <benchmark/benchmark.h>

#include "lc_mpmc_queue.h"
#include "lc_thread_pool.h"

using namespace lc;

static void BM_ThreadPoolSingleTask(benchmark::State &state) {
    auto queue = std::make_shared<
        MPMCQueue<Context<EmptyMetadata, std::function<void()>>>>(1024);
    ThreadPool<4> pool(queue);

    for (auto _ : state) {
        std::promise<void> promise;
        auto               future = promise.get_future();
        pool.submit([&promise]() { promise.set_value(); });
        future.wait();
    }
}

BENCHMARK(BM_ThreadPoolSingleTask);

static void cpu_work() {
    volatile int sum = 0;
    for (int i = 0; i < 10000; ++i) {
        sum += i;
    }
}

static void BM_ThreadPoolCPUIntensive(benchmark::State &state) {
    auto queue = std::make_shared<
        MPMCQueue<Context<EmptyMetadata, std::function<void()>>>>(4096);
    ThreadPool<8> pool(queue);
    int           task_count = state.range(0);

    for (auto _ : state) {
        std::vector<std::future<void>> results;
        for (int i = 0; i < task_count; ++i) {
            results.push_back(pool.submit([] { cpu_work(); }));
        }
        for (auto &f : results) {
            f.wait();
        }
    }
}

BENCHMARK(BM_ThreadPoolCPUIntensive)->Arg(10)->Arg(64)->Arg(500);

static void BM_ThreadPoolConcurrency(benchmark::State &state) {
    auto queue = std::make_shared<
        MPMCQueue<Context<EmptyMetadata, std::function<void()>>>>(8192);
    ThreadPool<16> pool(queue);
    int            task_count = state.range(0);

    for (auto _ : state) {
        std::atomic<int>   counter = 0;
        std::promise<void> promise;
        auto               future = promise.get_future();

        for (int i = 0; i < task_count; ++i) {
            pool.submit([&] {
                if (counter.fetch_add(1) + 1 == task_count) {
                    promise.set_value();
                }
            });
        }
        future.wait();
    }
}

BENCHMARK(BM_ThreadPoolConcurrency)->Arg(50)->Arg(64)->Arg(512)->Arg(2000);
