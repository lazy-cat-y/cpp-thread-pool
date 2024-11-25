
#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <vector>
#include "atomic-queue.h"

class AtomicQueueTest : public ::testing::Test {
 protected:
  tp::AtomicQueue<int> queue;

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(AtomicQueueTest, InitialState) {
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(queue.front(), std::nullopt);
  EXPECT_EQ(queue.dequeue(), std::nullopt);
}

TEST_F(AtomicQueueTest, EnqueueAndDequeue) {
  queue.enqueue(1);
  queue.enqueue(2);
  queue.enqueue(3);

  EXPECT_EQ(queue.size(), 3);
  EXPECT_EQ(queue.front(), std::optional<int>(1));

  EXPECT_EQ(queue.dequeue(), std::optional<int>(1));
  EXPECT_EQ(queue.size(), 2);

  EXPECT_EQ(queue.dequeue(), std::optional<int>(2));
  EXPECT_EQ(queue.dequeue(), std::optional<int>(3));

  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(queue.dequeue(), std::nullopt);
}

TEST_F(AtomicQueueTest, MultiThreadedEnqueueDequeue) {
  const int thread_count = 10;
  const int items_per_thread = 100;

  auto enqueue_task = [&]() {
    for (int i = 0; i < items_per_thread; ++i) {
      queue.enqueue(i);
    }
  };

  auto dequeue_task = [&]() {
    for (int i = 0; i < items_per_thread; ++i) {
      auto xx = queue.dequeue();
    }
  };

  std::vector<std::thread> threads;

  for (int i = 0; i < thread_count; ++i) {
    threads.emplace_back(enqueue_task);
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(queue.size(), thread_count * items_per_thread);

  threads.clear();

  for (int i = 0; i < thread_count; ++i) {
    threads.emplace_back(dequeue_task);
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(queue.size(), 0);
}

TEST_F(AtomicQueueTest, DequeueFromEmptyQueue) {
  EXPECT_EQ(queue.dequeue(), std::nullopt);  // 空队列出队返回 std::nullopt
  EXPECT_EQ(queue.front(), std::nullopt);  // 空队列前部也返回 std::nullopt
}

TEST_F(AtomicQueueTest, DifferentValueTypes) {
  tp::AtomicQueue<std::string> string_queue;

  string_queue.enqueue("hello");
  string_queue.enqueue(std::string("world"));

  EXPECT_EQ(string_queue.size(), 2);
  EXPECT_EQ(string_queue.front(), std::optional<std::string>("hello"));

  EXPECT_EQ(string_queue.dequeue(), std::optional<std::string>("hello"));
  EXPECT_EQ(string_queue.dequeue(), std::optional<std::string>("world"));
  EXPECT_EQ(string_queue.size(), 0);
}

TEST_F(AtomicQueueTest, StressTest) {
  const int num_elements = 1'000'000;

  for (int i = 0; i < num_elements; ++i) {
    queue.enqueue(i);
  }

  EXPECT_EQ(queue.size(), num_elements);

  for (int i = 0; i < num_elements; ++i) {
    EXPECT_EQ(queue.dequeue(), std::optional<int>(i));
  }

  EXPECT_EQ(queue.size(), 0);
}