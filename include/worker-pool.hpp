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

File: pool
*****************************************************************************/


/** @file /include/pool.hpp
 *  @brief Contains the Pool class.
 *  @author H. Y.
 *  @date 11/20/2024
 */

#ifndef POOL_H
#define POOL_H

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include "m-define.h"
#include "worker.hpp"

/**
 * @brief A thread pool implementation for managing and executing tasks across
 * multiple worker threads.
 * @tparam POOL_SIZE The number of worker threads in the pool.
 * @tparam QUEUE_SIZE The size of the task queue for each worker.
 * @tparam CHECK_TIME The interval in seconds for monitoring worker health and
 * detecting deadlocks.
 */
template <size_t POOL_SIZE = 10, size_t QUEUE_SIZE = 100, size_t CHECK_TIME = 5>
class ThreadPool {
 public:
  /**
   * @brief Constructs a new ThreadPool object, initializes workers, and starts
   * monitoring tasks.
   */
  ThreadPool() : _next_worker(0), _m_status(THREAD_POOL_STATUS_IDLE) {
    for (size_t i = 0; i < POOL_SIZE; ++i) {
      _m_workers.push_back(std::make_unique<Worker>());
    }
    for (auto &worker : _m_workers) {
      worker->start_pool(_m_workers);
    }
    _m_dead_worker = std::make_unique<Worker>();
    _m_dead_worker->start();
    _m_heartbeat_monitor_worker = std::make_unique<Worker>();
    _m_heartbeat_monitor_worker->start();
    submit_monitor_taskers();
    _m_status.exchange(THREAD_POOL_STATUS_RUNNING);
  }

  /* Deleted copy constructor to prevent copying. */
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;

  /**
   * @brief Destructor that stops all workers, shuts down monitors, and cleans
   * up resources.
   */
  ~ThreadPool() {
    _m_status.exchange(THREAD_POOL_STATUS_STOPPING);
    for (auto &worker : _m_workers) {
      worker->stop();
      worker->join();
    }
    close_monitor_taskers();
    _m_status.exchange(THREAD_POOL_STATUS_STOPPED);
  }

  /**
   * @brief Submits a task to the thread pool for execution.
   * @tparam Func The type of the function to execute.
   * @tparam Args The types of the function arguments.
   * @param func The function to execute.
   * @param args The arguments to the function.
   * @return A future representing the result of the task.
   */
  template <typename Func, typename... Args>
  std::future<typename std::invoke_result_t<Func, Args...>> submit(
      Func &&func, Args &&...args);

  /**
   * @brief Shuts down the thread pool, stopping all workers and monitors.
   */
  void shutdown();

  /**
   * @brief Retrieves the current status of the thread pool.
   * @return The thread pool's status as an integer.
   */
  int status() const { return _m_status; }

#if !defined(ENABLE_TEST)
 private:
#endif
  /**
   * @brief Submits monitoring tasks for deadlock detection and heartbeat
   * monitoring.
   */
  void submit_monitor_taskers();

  /**
   * @brief Closes the monitoring tasks and joins the monitor threads.
   */
  void close_monitor_taskers();

  /**
   * @brief Restarts a specific worker in the pool.
   * @param worker_id The ID of the worker to restart.
   * @throw std::out_of_range if the worker ID is invalid.
   */
  void restart_worker(size_t worker_id);

  /**
   * @brief A vector of unique pointers to the worker threads in the pool.
   */
  std::vector<std::unique_ptr<Worker>> _m_workers;

  /**
   * @brief The ID of the next worker to receive a task, managed in a
   * round-robin fashion.
   */
  std::atomic<size_t> _next_worker;

  /**
   * @brief The current status of the thread pool.
   */
  std::atomic<uint8_t> _m_status;

  /**
   * @brief A mutex for synchronizing access to the thread pool's status.
   */
  std::mutex _m_status_mutex;

  /**
   * @brief A dedicated worker for detecting deadlocks in the thread pool.
   */
  std::unique_ptr<Worker> _m_dead_worker;

  /**
   * @brief A dedicated worker for monitoring worker heartbeats to detect dead
   * workers.
   */
  std::unique_ptr<Worker> _m_heartbeat_monitor_worker;
};

template <size_t POOL_SIZE, size_t QUEUE_SIZE, size_t CHECK_TIME>
template <typename Func, typename... Args>
std::future<typename std::invoke_result_t<Func, Args...>>
ThreadPool<POOL_SIZE, QUEUE_SIZE, CHECK_TIME>::submit(Func &&func,
                                                      Args &&...args) {
  if (_m_status.load(std::memory_order_relaxed) != THREAD_POOL_STATUS_RUNNING) {
    throw std::runtime_error("Thread pool is not running.");
  }

  using ReturnType = typename std::invoke_result_t<Func, Args...>;

  std::packaged_task<ReturnType()> task(
      [func = std::forward<Func>(func),
       args = std::make_tuple(std::forward<Args>(args)...)]() {
        if constexpr (std::is_same_v<ReturnType, void>) {
          std::apply(func, args);
        } else {
          return std::apply(func, args);
        }
      });

  auto future = task.get_future();

  size_t worker_id =
      _next_worker.fetch_add(1, std::memory_order_relaxed) % POOL_SIZE;

  _m_workers[worker_id]->submit(std::move(task));
  return future;
}

template <size_t POOL_SIZE, size_t QUEUE_SIZE, size_t CHECK_TIME>
void ThreadPool<POOL_SIZE, QUEUE_SIZE, CHECK_TIME>::shutdown() {
  _m_status.exchange(THREAD_POOL_STATUS_STOPPING);
  for (auto &worker : _m_workers) {
    worker->stop();
  }
  for (auto &worker : _m_workers) {
    worker->join();
  }
  close_monitor_taskers();
  _m_status.exchange(THREAD_POOL_STATUS_STOPPED);
}

template <size_t POOL_SIZE, size_t QUEUE_SIZE, size_t CHECK_TIME>
void ThreadPool<POOL_SIZE, QUEUE_SIZE, CHECK_TIME>::submit_monitor_taskers() {
  _m_dead_worker->submit([this]() {
    while (_m_status == THREAD_POOL_STATUS_RUNNING) {
      std::this_thread::sleep_for(std::chrono::seconds(CHECK_TIME));

      bool all_idle = true;
      bool task_queue_empty = true;

      {
        std::lock_guard<std::mutex> lock(_m_status_mutex);

        all_idle =
            std::all_of(_m_workers.begin(), _m_workers.end(),
                        [](const auto &worker) { return worker->is_idle(); });

        task_queue_empty = std::all_of(
            _m_workers.begin(), _m_workers.end(),
            [](const auto &worker) { return worker->task_queue_empty(); });
      }

      if (all_idle && !task_queue_empty) {
#if defined(DEBUG)
        std::cerr << "Deadlock detected: all workers idle but queue not empty."
                  << std::endl;
#endif
        shutdown();
        throw std::runtime_error("Deadlock detected.");
      }
    }
  });

  _m_heartbeat_monitor_worker->submit([this]() {
    while (_m_status == THREAD_POOL_STATUS_RUNNING) {
      std::this_thread::sleep_for(std::chrono::seconds(CHECK_TIME));

      auto now = std::chrono::steady_clock::now();

      for (size_t i = 0; i < POOL_SIZE; ++i) {
        const auto &worker = _m_workers[i];
        if ((now - worker->last_heartbeat()) >
            std::chrono::seconds(CHECK_TIME * 2)) {
#if defined(DEBUG)
          std::cerr << "Worker " << i << " is dead." << std::endl;
#endif
          restart_worker(i);
        }
      }
    }
  });
}

template <size_t POOL_SIZE, size_t QUEUE_SIZE, size_t CHECK_TIME>
void ThreadPool<POOL_SIZE, QUEUE_SIZE, CHECK_TIME>::close_monitor_taskers() {
  _m_dead_worker->stop();
  _m_dead_worker->join();
  _m_heartbeat_monitor_worker->stop();
  _m_heartbeat_monitor_worker->join();
}

template <size_t POOL_SIZE, size_t QUEUE_SIZE, size_t CHECK_TIME>
void ThreadPool<POOL_SIZE, QUEUE_SIZE, CHECK_TIME>::restart_worker(
    size_t worker_id) {
  if (worker_id >= POOL_SIZE) {
    throw std::out_of_range("Worker ID out of range.");
  }

  std::lock_guard<std::mutex> lock(_m_status_mutex);

  _m_workers[worker_id]->stop();
  _m_workers[worker_id]->join();
  _m_workers[worker_id] = std::make_unique<Worker>();

  _m_workers[worker_id]->start_pool(_m_workers);
}

#endif  // POOL_H
