
#include <benchmark/benchmark.h>
#include "lc_thread_pool.h"  // 替换为你的线程池头文件
#include "lc_mpmc_queue.h"   // 替换为你的 queue 实现

using namespace lc;

static void BM_ThreadPoolSingleTask(benchmark::State& state) {
    auto queue = std::make_shared<MPMCQueue<Context<EmptyMetadata, std::function<void()>>>>(1024);
    ThreadPool<4> pool(queue);

    for (auto _ : state) {
        std::promise<void> promise;
        auto future = promise.get_future();
        pool.submit([&promise]() { promise.set_value(); });
        future.wait();  // 等待任务执行完成
    }
}
BENCHMARK(BM_ThreadPoolSingleTask);