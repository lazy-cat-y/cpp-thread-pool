
/** @file /test/test-channel.cc
 *  @brief Contains the Worker class test cases.
 *  @author H. Y.
 *  @date 11/19/2024
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include "m-define.h"
#include "worker.hpp"

TEST(WorkerTest, Initialization) {
  Worker worker;
  EXPECT_EQ(worker.status(), WORKER_STATUS_CREATED);
  EXPECT_FALSE(worker.is_idle());
}

TEST(WorkerTest, StartAndStop) {
  Worker worker;
  EXPECT_NO_THROW(worker.start());
  EXPECT_EQ(worker.status(), WORKER_STATUS_IDLE);

  EXPECT_NO_THROW(worker.stop());
  EXPECT_EQ(worker.status(), WORKER_STATUS_STOPPING);
  EXPECT_NO_THROW(worker.join());
  EXPECT_EQ(worker.status(), WORKER_STATUS_STOPPED);
}

TEST(WorkerTest, SubmitTask) {
  Worker worker;
  worker.start();

  auto task = []() { return 42; };
  auto future = worker.submit(task);

  EXPECT_EQ(future.get(), 42);

  worker.stop();
  worker.join();
}

TEST(WorkerTest, RunMultipleTasks) {
  Worker worker;
  worker.start();

  auto task1 = []() { return 1; };
  auto task2 = []() { return 2; };

  auto future1 = worker.submit(task1);
  auto future2 = worker.submit(task2);

  EXPECT_EQ(future1.get(), 1);
  EXPECT_EQ(future2.get(), 2);

  worker.stop();
  worker.join();
}

TEST(WorkerTest, WorkerSleepFor) {
  Worker worker;
  worker.start();

  auto start_time = std::chrono::steady_clock::now();
  worker.sleep_for(std::chrono::milliseconds(200));

  worker.submit([]() {}).get();  // Submit a no-op task.

  auto elapsed_time = std::chrono::steady_clock::now() - start_time;
  EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time)
                .count(),
            200);

  worker.stop();
  worker.join();
}

TEST(WorkerTest, StartPool) {
  std::vector<std::unique_ptr<Worker>> workers;
  for (int i = 0; i < 4; ++i) {
    workers.push_back(std::make_unique<Worker>());
  }

  for (auto &worker : workers) {
    worker->start_pool(workers);
  }

  auto task = []() { return 42; };
  auto future = workers[0]->submit(task);

  EXPECT_EQ(future.get(), 42);

  for (auto &worker : workers) {
    worker->stop();
    worker->join();
  }
}

TEST(WorkerTest, HandleClosedChannel) {
  Worker worker;
  worker.start();

  worker.stop();
  worker.join();

  EXPECT_THROW(worker.submit([]() {}), std::runtime_error);
}

TEST(WorkerTest, Heartbeat) {
  Worker worker;
  worker.start();

  auto start_time = std::chrono::steady_clock::now();

  worker.submit([]() {}).get();  // Submit a no-op task.

  auto elapsed_time = std::chrono::steady_clock::now() - start_time;
  auto heartbeat = worker.last_heartbeat();

  // heartbeat greater than start_time
  EXPECT_GE(heartbeat, start_time);

  worker.stop();
  worker.join();
}