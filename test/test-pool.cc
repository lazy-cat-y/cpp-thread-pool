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


/** @file /test/test-pool.cc
 *  @brief Contains the ThreadPool class test cases.
 *  @author H. Y.
 *  @date 11/20/2024
 */

#include <gtest/gtest.h>
#include <future>
#include <vector>
#include "pool.hpp"  

TEST(ThreadPoolTest, ConstructorAndDestructor) {
    EXPECT_NO_THROW({
        ThreadPool pool;
    });
}


TEST(ThreadPoolTest, TaskSubmission) {
    ThreadPool<4, 10, 2> pool;

    auto task = pool.submit([](int a, int b) { return a + b; }, 3, 5);
    ASSERT_EQ(task.get(), 8);
}

TEST(ThreadPoolTest, MultipleTasks) {
    ThreadPool<4, 10, 2> pool;

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([](int x) { return x * x; }, i));
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i * i); 
    }
}

TEST(ThreadPoolTest, CannotSubmitAfterShutdown) {
    ThreadPool<4, 10, 2> pool;

    pool.shutdown();

    EXPECT_THROW(pool.submit([]() {}), std::runtime_error);
}

TEST(ThreadPoolTest, Status) {
    ThreadPool<4, 10, 2> pool;

    EXPECT_EQ(pool.status(), THREAD_POOL_STATUS_RUNNING);

    pool.shutdown(); // 关闭线程池
    EXPECT_EQ(pool.status(), THREAD_POOL_STATUS_STOPPED);
}

TEST(ThreadPoolTest, LargeTaskQueue) {
    ThreadPool<4, 100, 2> pool;

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([](int x) { return x * x; }, i));
    }

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

TEST(ThreadPoolTest, RestartWorker) {
    ThreadPool<4, 10, 2> pool;

    auto task = pool.submit([]() { return 42; });
    EXPECT_EQ(task.get(), 42);

    pool.restart_worker(0);

    auto new_task = pool.submit([]() { return 24; });
    EXPECT_EQ(new_task.get(), 24);
}