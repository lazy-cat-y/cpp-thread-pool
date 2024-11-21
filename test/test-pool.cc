
#include <gtest/gtest.h>
#include <atomic>
#include <future>
#include <vector>
#include "pool.hpp"  // 假设你的 ThreadPool 类定义在这个文件中

// 基础测试：单个任务
TEST(ThreadPoolTest, SingleTaskExecution) {
  ThreadPool<2, 10> pool;

  // 提交一个简单任务，返回一个整数
  auto result = pool.submit([]() { return 42; });

  // 验证任务结果
  EXPECT_EQ(result.get(), 42);
}

// 多任务测试
TEST(ThreadPoolTest, MultipleTasksExecution) {
  ThreadPool<4, 20> pool;
  std::vector<std::future<int>> futures;

  // 提交多个任务
  for (int i = 0; i < 10; ++i) {
    futures.push_back(pool.submit([i]() { return i * i; }));
  }

  // 验证每个任务的结果
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(futures[i].get(), i * i);
  }
}

// 并发任务测试
TEST(ThreadPoolTest, ConcurrentTasksExecution) {
  constexpr size_t TASK_COUNT = 100;
  ThreadPool<8, 100> pool;
  std::atomic<int> counter{0};

  // 提交多个任务，增加计数器
  std::vector<std::future<void>> futures;
  for (size_t i = 0; i < TASK_COUNT; ++i) {
    futures.push_back(pool.submit([&counter]() { counter.fetch_add(1); }));
  }

  // 等待所有任务完成
  for (auto &future : futures) {
    future.get();
  }

  // 验证计数器值
  EXPECT_EQ(counter.load(), TASK_COUNT);
}

// 异常处理测试
TEST(ThreadPoolTest, TaskExceptionHandling) {
  ThreadPool<2, 10> pool;

  // 提交一个会抛出异常的任务
  auto future = pool.submit([]() -> int { throw std::runtime_error("Error"); });

  // 验证捕获异常
  EXPECT_THROW(future.get(), std::runtime_error);
}

// 线程池状态测试
TEST(ThreadPoolTest, ThreadPoolStatus) {
  ThreadPool<3, 10> pool{};

  EXPECT_NO_THROW(pool.submit([]() {}));
  
  pool.shutdown();
  EXPECT_THROW(pool.submit([]() {}), std::runtime_error);
}
