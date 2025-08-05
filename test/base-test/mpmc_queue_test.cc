
#include <gtest/gtest.h>

#include "lc_mpmc_queue.h"

using namespace lc;

TEST(MPMCQueueTest, EnqueueDequeueSingleThread) {
    MPMCQueue<int> queue(8);

    EXPECT_TRUE(queue.enqueue(1));
    EXPECT_TRUE(queue.enqueue(2));
    EXPECT_TRUE(queue.enqueue(3));

    int out;
    EXPECT_TRUE(queue.dequeue(out));
    EXPECT_EQ(out, 1);

    EXPECT_TRUE(queue.dequeue(out));
    EXPECT_EQ(out, 2);

    EXPECT_TRUE(queue.dequeue(out));
    EXPECT_EQ(out, 3);
}

TEST(MPMCQueueTest, DequeueEmptyReturnsFalse) {
    MPMCQueue<int> queue(4);
    int            out;
    EXPECT_FALSE(queue.dequeue(out));
}

TEST(MPMCQueueTest, EnqueueFullReturnsFalse) {
    MPMCQueue<int> queue(2);
    EXPECT_TRUE(queue.enqueue(10));
    EXPECT_TRUE(queue.enqueue(20));
    EXPECT_FALSE(queue.enqueue(30));  // Should be full
}

TEST(MPMCQueueTest, MoveOnlyTypeWorks) {
    MPMCQueue<std::unique_ptr<int>> queue(2);

    EXPECT_TRUE(queue.enqueue(std::make_unique<int>(42)));
    std::unique_ptr<int> out;
    EXPECT_TRUE(queue.dequeue(out));
    ASSERT_TRUE(out);
    EXPECT_EQ(*out, 42);
}

TEST(MPMCQueueTest, MultiThreadedEnqueueDequeue) {
    constexpr size_t queue_size       = 1024;
    constexpr size_t num_threads      = 4;
    constexpr size_t items_per_thread = 1000;

    MPMCQueue<int>      queue(queue_size);
    std::atomic<size_t> success_count {0};

    // Producer threads
    std::vector<std::thread> producers;
    for (size_t i = 0; i < num_threads; ++i) {
        producers.emplace_back([&queue, &success_count, i]() {
            for (size_t j = 0; j < items_per_thread; ++j) {
                int value = static_cast<int>(i * items_per_thread + j);
                while (!queue.enqueue(value)) {}  // busy-wait
                success_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Consumer thread
    std::set<int> received;
    std::thread   consumer([&]() {
        int    value;
        size_t total = num_threads * items_per_thread;
        while (received.size() < total) {
            if (queue.dequeue(value)) {
                received.insert(value);
            }
        }
    });

    for (auto &t : producers) {
        t.join();
    }
    consumer.join();

    EXPECT_EQ(received.size(), num_threads * items_per_thread);
}
