
/** @file /test/test-channel.cc
 *  @brief Contains the Channel class test cases.
 *  @author H. Y.
 *  @date 11/19/2024
 */



#include <thread>
#include <vector>
#include "channel.hpp"
#include "gtest/gtest.h"

class ChannelTest : public ::testing::Test {
 protected:
  Channel<int, 10> channel;
};

TEST_F(ChannelTest, Initializtion) {
  EXPECT_FALSE(channel.is_closed());
  EXPECT_EQ(channel.size(), 0);
}

TEST_F(ChannelTest, SendReceive) {
  channel.send(42);
  EXPECT_EQ(channel.size(), 1);

  auto value = channel.receive();
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(value.value(), 42);

  EXPECT_EQ(channel.size(), 0);
}

TEST_F(ChannelTest, Close) {
  channel.send(42);
  EXPECT_FALSE(channel.is_closed());

  channel.close();
  EXPECT_TRUE(channel.is_closed());

  auto value = channel.receive();
  EXPECT_FALSE(value.has_value());
}

TEST_F(ChannelTest, OperatorOverloads) {
  channel << std::optional<int>(10);
  EXPECT_EQ(channel.size(), 1);
  std::optional<int> value;
  channel >> value;

  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(value.value(), 10);
  EXPECT_EQ(channel.size(), 0);

  channel << std::optional<int>(20);
  EXPECT_EQ(channel.size(), 1);
  auto value2 = channel();
  EXPECT_EQ(channel.size(), 0);
  EXPECT_TRUE(value2.has_value());
  EXPECT_EQ(value2.value(), 20);
}

TEST_F(ChannelTest, ThreadSafety) {
  const int NUM_THREADS = 10;
  const int NUM_VALUES = 100;

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  std::vector<int> received_values;
  std::mutex result_mutex;

  for (int i = 0; i < NUM_THREADS; ++i) {
    producers.emplace_back([&] {
      for (int j = 0; j < NUM_VALUES; ++j) {
        channel.send(j);
      }
    });
  }

  for (int i = 0; i < NUM_THREADS; ++i) {
    consumers.emplace_back([&]() {
      while (true) {
        auto value = channel.receive();
        if (!value.has_value()) break;                   
        std::lock_guard<std::mutex> guard(result_mutex);  
        received_values.push_back(value.value());
      }
    });
  }

  // Wait for all threads to finish
  for (auto &thr : producers) thr.join();
  channel.close();
  for (auto &thr : consumers) thr.join();

  EXPECT_EQ(received_values.size(), NUM_THREADS * NUM_VALUES);
}
