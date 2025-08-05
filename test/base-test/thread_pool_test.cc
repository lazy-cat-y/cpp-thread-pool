
#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <thread>

#include "lc_thread_pool.h"

using namespace std::chrono_literals;
using namespace lc;

struct TestMetadata {
    int priority;
};

TEST(ThreadPoolTest, BasicVoidTask) {
    using Task = Context<TestMetadata, std::function<void()>>;
    auto                        queue = std::make_shared<MPMCQueue<Task>>(128);
    ThreadPool<4, TestMetadata> pool(queue);

    std::atomic<int> counter = 0;

    for (int i = 0; i < 10; ++i) {
        pool.submit(TestMetadata {.priority = i}, [&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    std::this_thread::sleep_for(200ms);
    pool.shutdown();

    EXPECT_EQ(counter.load(), 10);
}

TEST(ThreadPoolTest, TaskWithReturnValue) {
    using Task = Context<TestMetadata, std::function<void()>>;
    auto                        queue = std::make_shared<MPMCQueue<Task>>(128);
    ThreadPool<4, TestMetadata> pool(queue);

    auto fut =
        pool.submit(TestMetadata {.priority = 1}, []() -> int { return 42; });

    EXPECT_EQ(fut.get(), 42);

    pool.shutdown();
}

TEST(ThreadPoolTest, TaskWithArguments) {
    using Task = Context<TestMetadata, std::function<void()>>;
    auto                        queue = std::make_shared<MPMCQueue<Task>>(128);
    ThreadPool<4, TestMetadata> pool(queue);

    auto fut =
        pool.submit(TestMetadata {.priority = 3}, [](int a, int b) -> int {
        return a + b;
    }, 7, 5);

    EXPECT_EQ(fut.get(), 12);

    pool.shutdown();
}

TEST(ThreadPoolTest, ManyTasks) {
    using Task = Context<TestMetadata, std::function<void()>>;
    auto                        queue = std::make_shared<MPMCQueue<Task>>(1024);
    ThreadPool<8, TestMetadata> pool(queue);

    std::atomic<int> sum        = 0;
    constexpr int    kTaskCount = 1000;

    for (int i = 0; i < kTaskCount; ++i) {
        pool.submit(TestMetadata {.priority = 0},
                    [&sum]() { sum.fetch_add(1, std::memory_order_relaxed); });
    }

    std::this_thread::sleep_for(500ms);
    pool.shutdown();

    EXPECT_EQ(sum.load(), kTaskCount);
}
