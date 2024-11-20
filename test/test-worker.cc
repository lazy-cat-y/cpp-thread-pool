

#include <gtest/gtest.h>
#include <stdexcept>
#include "worker.hpp"

class WorkerTest : public ::testing::Test {
 protected:
  void SetUp() override { worker.start(); }

  void TearDown() override { worker.stop(); }

  Worker worker;
};

TEST_F(WorkerTest, StartAndStopWorker) {
  EXPECT_NO_THROW(worker.start());

  worker.stop();
  EXPECT_NO_THROW(worker.stop());
}

TEST_F(WorkerTest, SubmitTask) {
  auto future = worker.submit([]() { return 42; });
  ASSERT_EQ(future.get(), 42);

  std::atomic<int> counter = 0;
  for (int i = 0; i < 10; ++i) {
    auto future = worker.submit([&counter]() { counter++; });
  }

  // 等待任务执行完成
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(counter, 10);
}

TEST_F(WorkerTest, SubmitTaskAfterStop) {
  worker.stop();
  EXPECT_THROW(worker.submit([]() { return 42; }), std::runtime_error);
}

TEST_F(WorkerTest, JoinWorker) {
  worker.stop();
  EXPECT_NO_THROW(worker.join()); 
}

TEST_F(WorkerTest, RunMultipleTasks) {
  std::vector<std::future<int>> futures;

  for (int i = 0; i < 5; ++i) {
    futures.push_back(worker.submit([i]() { return i * i; }));
  }

  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(futures[i].get(), i * i);
  }
}

TEST_F(WorkerTest, WorkerReuse) {
  worker.stop();
  EXPECT_THROW(worker.start(), std::runtime_error);
}