/*****************************************************************************
Copyright (c) 2024 Haoxin Yang

Author: Haoxin Yang (lazy-cat-y)

Licensed under the MIT License. You may obtain a copy of the License at:
https://opensource.org/licenses/MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

File: test-pool
*****************************************************************************/


/** @file /test/benchmark/test-performance
 *  @brief Contains the ThreadPool class test cases.
 *  @author H. Y.
 *  @date 11/22/2024
 */

#include <benchmark/benchmark.h>
#include "worker-pool.hpp" // 包含线程池的实现文件
#include <vector>
#include <numeric> // std::accumulate
#include <future>  // std::future

// 测试1：单任务执行时间
static void BM_ThreadPoolSingleTask(benchmark::State& state) {
    ThreadPool<4, 10> pool; // 创建线程池
    for (auto _ : state) {
        // 任务：简单计算
        auto future = pool.submit([](int a, int b) { return a + b; }, 3, 4);
        benchmark::DoNotOptimize(future.get());
    }
}
BENCHMARK(BM_ThreadPoolSingleTask);

// 测试2：提交多个任务
static void BM_ThreadPoolMultipleTasks(benchmark::State& state) {
    ThreadPool<4, 100> pool; // 创建线程池
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        for (int i = 0; i < state.range(0); ++i) {
            futures.push_back(pool.submit([](int x) { return x * x; }, i));
        }
        for (auto& future : futures) {
            benchmark::DoNotOptimize(future.get());
        }
    }
}
// 任务数从 10 增加到 1000 测试
BENCHMARK(BM_ThreadPoolMultipleTasks)->Range(10, 1000);

// 测试3：线程池并发性能
static void BM_ThreadPoolConcurrency(benchmark::State& state) {
    ThreadPool<8, 1000> pool; // 创建8线程的线程池
    for (auto _ : state) {
        std::vector<std::future<void>> futures;
        for (int i = 0; i < state.range(0); ++i) {
            futures.push_back(pool.submit([]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }));
        }
        for (auto& future : futures) {
            future.get(); // 等待任务完成
        }
    }
}
// 模拟并发任务数从 50 到 2000 测试
BENCHMARK(BM_ThreadPoolConcurrency)->Range(50, 2000);

// 测试4：CPU密集型任务
static void BM_ThreadPoolCPUIntensive(benchmark::State& state) {
    ThreadPool<8, 100> pool; // 创建8线程的线程池
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        for (int i = 0; i < state.range(0); ++i) {
            futures.push_back(pool.submit([]() {
                int sum = 0;
                for (int i = 0; i < 10000; ++i) {
                    sum += i * i;
                }
                return sum;
            }));
        }
        for (auto& future : futures) {
            benchmark::DoNotOptimize(future.get());
        }
    }
}
// 模拟任务数从 10 到 500
BENCHMARK(BM_ThreadPoolCPUIntensive)->Range(10, 500);