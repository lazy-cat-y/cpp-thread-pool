
#include <gtest/gtest.h>
#include <thread>
#include "atomic-queue.h"

TEST(AtomicQueueTest, EmptyOnInit) {
    tp::AtomicQueue<int> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(AtomicQueueTest, SingleThreadPushPop) {
    tp::AtomicQueue<int> q;
    EXPECT_TRUE(q.empty());

    q.push(42);
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(q.size(), 1u);
    EXPECT_EQ(q.front(), 42);
    EXPECT_EQ(q.back(), 42);

    auto val = q.pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 42);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(AtomicQueueTest, MultipleElements) {
    tp::AtomicQueue<int> q;
    for (int i = 0; i < 10; ++i) {
        q.push(i);
    }

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(q.size(), 10u);
    EXPECT_EQ(q.front(), 0);
    EXPECT_EQ(q.back(), 9);

    for (int i = 0; i < 10; ++i) {
        auto val = q.pop();
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(val.value(), i);
    }

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(AtomicQueueTest, Clear) {
    tp::AtomicQueue<int> q;
    for (int i = 0; i < 5; ++i) {
        q.push(i);
    }
    EXPECT_EQ(q.size(), 5u);
    q.clear();
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(AtomicQueueTest, ConcurrentPushPop) {
    tp::AtomicQueue<int> q;
    const int num_threads = 4;
    const int num_per_thread = 1000;

    auto pusher = [&q, num_per_thread]() {
        for (int i = 0; i < num_per_thread; ++i) {
            q.push(i);
        }
    };

    auto popper = [&q, num_per_thread]() {
        int count = 0;
        while (count < num_per_thread) {
            auto val = q.pop();
            if (val.has_value()) {
                count++;
            } else {
                // 如果没有弹出元素则稍微等待一下
                std::this_thread::yield();
            }
        }
    };

    // 启动2个线程push，2个线程pop
    std::thread t1(pusher);
    std::thread t2(pusher);
    std::thread t3(popper);
    std::thread t4(popper);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // 所有元素应该已被pop完
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}