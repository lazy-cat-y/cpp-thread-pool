
#include <gtest/gtest.h>
#include <future>
#include <vector>
#include "pool.hpp"  // 假设你的 ThreadPool 类定义在这个文件中

TEST(ThreadPoolTest, ConstructorAndDestructor) {
    EXPECT_NO_THROW({
        ThreadPool pool;
    });
}


// 测试任务提交和执行结果
TEST(ThreadPoolTest, TaskSubmission) {
    ThreadPool<4, 10, 2> pool;

    // 提交任务: 计算累加和
    auto task = pool.submit([](int a, int b) { return a + b; }, 3, 5);
    ASSERT_EQ(task.get(), 8); // 验证任务返回的结果
}

// 测试多任务执行
TEST(ThreadPoolTest, MultipleTasks) {
    ThreadPool<4, 10, 2> pool;

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([](int x) { return x * x; }, i));
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i * i); // 验证每个任务的结果
    }
}

// 测试线程池关闭后无法提交任务
TEST(ThreadPoolTest, CannotSubmitAfterShutdown) {
    ThreadPool<4, 10, 2> pool;

    pool.shutdown(); // 关闭线程池

    EXPECT_THROW(pool.submit([]() {}), std::runtime_error); // 提交任务时抛出异常
}

// 测试线程池状态
TEST(ThreadPoolTest, Status) {
    ThreadPool<4, 10, 2> pool;

    EXPECT_EQ(pool.status(), THREAD_POOL_STATUS_RUNNING); // 验证线程池状态为运行中

    pool.shutdown(); // 关闭线程池
    EXPECT_EQ(pool.status(), THREAD_POOL_STATUS_STOPPED); // 验证线程池状态为停止
}

// 测试大任务队列执行
TEST(ThreadPoolTest, LargeTaskQueue) {
    ThreadPool<4, 100, 2> pool;

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([](int x) { return x * x; }, i));
    }

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(futures[i].get(), i * i); // 验证每个任务的结果
    }
}

// 测试线程池恢复工作后任务执行
TEST(ThreadPoolTest, RestartWorker) {
    ThreadPool<4, 10, 2> pool;

    auto task = pool.submit([]() { return 42; });
    EXPECT_EQ(task.get(), 42);

    // 模拟某个 worker 崩溃
    pool.restart_worker(0);

    // 验证任务仍可提交并正常执行
    auto new_task = pool.submit([]() { return 24; });
    EXPECT_EQ(new_task.get(), 24);
}