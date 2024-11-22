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

File: worker
*****************************************************************************/


/** @file /include/work.hpp
 *  @brief Contains the Worker class.
 *  @author H. Y.
 *  @date 11/19/2024
 */

#ifndef WORKER_H
#define WORKER_H

#include <cassert>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>
#include <vector>
#include "channel.hpp"
#include "m-define.h"

/**
 * @brief A worker thread class for executing tasks and managing a task queue.
 */
class Worker {
 public:
  /**
   * @brief Constructs a new Worker object with default status and paused time.
   */
  Worker()
      : _m_status{WORKER_STATUS_CREATED},
        _m_paused_until{std::chrono::steady_clock::now()} {}

  /** Deleted copy constructor to prevent copying. */
  Worker(const Worker &) = delete;
  Worker &operator=(const Worker &) = delete;
  Worker(Worker &&) = delete;
  Worker &operator=(Worker &&) = delete;

  /**
   * @brief Destructor that stops the worker and joins the thread.
   */
  ~Worker() {
    stop();
    join();
  }

  /**
   * @brief Starts the worker thread.
   */
  inline void start();

  /**
   * @brief Starts a pool of workers.
   * @param _m_all_workers A reference to the vector of all worker instances.
   */
  inline void start_pool(std::vector<std::unique_ptr<Worker>> &_m_all_workers);

  /**
   * @brief Stops the worker thread and closes the task queue.
   */
  inline void stop();

  /**
   * @brief Joins the worker thread if joinable.
   */
  inline void join();

  /**
   * @brief The main execution loop for the worker thread.
   */
  inline void run();

  /**
   * @brief The execution loop for a pool of worker threads.
   * @param workers A reference to the vector of all worker instances.
   */
  inline void run_pool(std::vector<std::unique_ptr<Worker>> &workers);

  /**
   * @brief Submits a task to the worker's task queue.
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
   * @brief Attempts to steal a task from the worker's task queue.
   * @return An optional packaged task if available.
   */
  inline std::optional<std::packaged_task<void()>> steal_task();

  /**
   * @brief Retrieves the current status of the worker.
   * @return The worker's status.
   */
  inline int status() const { return _m_status; }

  /**
   * @brief Gets a pointer to the current worker instance.
   * @return A const pointer to the worker.
   */
  const Worker *get() const { return this; }

  /**
   * @brief Checks if the worker is idle.
   * @return True if the worker is idle, false otherwise.
   */
  inline bool is_idle() const { return _m_status == WORKER_STATUS_IDLE; }

  /**
   * @brief Checks if the worker's task queue is empty.
   * @return True if the task queue is empty, false otherwise.
   */
  inline bool task_queue_empty() { return _m_channel.size() == 0; }

  /**
   * @brief Gets the last recorded heartbeat time of the worker.
   * @return The last heartbeat time point.
   */
  inline std::chrono::steady_clock::time_point last_heartbeat() const {
    return _m_heartbeat;
  }

  /**
   * @brief Puts the worker to sleep for a specified duration.
   * @param _m_time The duration to sleep.
   */
  inline void sleep_for(std::chrono::milliseconds _m_time);

 private:
  /**
   * @brief The thread associated with the worker.
   */
  std::thread _m_thread;

  /**
   * @brief The current status of the worker.
   */
  uint8_t _m_status;

  /**
   * @brief A mutex for synchronizing access to the worker's status.
   */
  std::mutex _m_status_mutex;

  /**
   * @brief The time until which the worker is paused.
   */
  std::chrono::steady_clock::time_point _m_paused_until;

  /**
   * @brief The last recorded heartbeat time of the worker.
   */
  std::chrono::steady_clock::time_point _m_heartbeat;

  /**
   * @brief The task queue for the worker.
   */
  Channel<std::packaged_task<void()>, 100> _m_channel;
};

template <typename Func, typename... Args>
std::future<typename std::invoke_result_t<Func, Args...>> Worker::submit(
    Func &&func, Args &&...args) {
  using ReturnType = typename std::invoke_result_t<Func, Args...>;

  std::packaged_task<ReturnType()> task(
      [func = std::forward<Func>(func),
       args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        return std::apply(std::move(func), std::move(args));
      });

  auto future = task.get_future();

  _m_channel << std::make_optional<std::packaged_task<void()>>(std::move(task));
  return future;
}

inline void Worker::start_pool(
    std::vector<std::unique_ptr<Worker>> &_m_all_workers) {
  if (_m_status == WORKER_STATUS_STOPPED ||
      _m_status == WORKER_STATUS_STOPPING) {
    throw std::runtime_error("Worker is stopping or stopped.");
  }
  if (_m_status == WORKER_STATUS_RUNNING || _m_status == WORKER_STATUS_IDLE) {
    return;
  }

  _m_status = WORKER_STATUS_IDLE;
  _m_thread = std::thread(&Worker::run_pool, this, std::ref(_m_all_workers));
}

inline void Worker::start() {
  if (_m_status == WORKER_STATUS_STOPPED ||
      _m_status == WORKER_STATUS_STOPPING) {
    throw std::runtime_error("Worker is stopping or stopped.");
  }
  if (_m_status == WORKER_STATUS_RUNNING || _m_status == WORKER_STATUS_IDLE) {
    return;
  }

  _m_status = WORKER_STATUS_IDLE;
  _m_thread = std::thread(&Worker::run, this);
}

inline void Worker::stop() {
  if (_m_status == WORKER_STATUS_STOPPED) {
    return;
  }

  _m_status = WORKER_STATUS_STOPPING;
  _m_channel.close();
}

inline void Worker::join() {
  if (!_m_thread.joinable()) return;
  _m_thread.join();
  _m_status = WORKER_STATUS_STOPPED;
}

inline void Worker::run_pool(
    std::vector<std::unique_ptr<Worker>> &_m_all_workers) {
  std::optional<std::packaged_task<void()>> o_task;
  while (!_m_channel.is_closed() && _m_status == WORKER_STATUS_IDLE) {
    std::unique_lock<std::mutex> lock(_m_status_mutex);
    if (_m_paused_until > std::chrono::steady_clock::now()) {
      lock.unlock();
      std::this_thread::sleep_for(_m_paused_until -
                                  std::chrono::steady_clock::now());
      continue;
    }
    lock.unlock();

    _m_status = WORKER_STATUS_RUNNING;
    _m_channel >> o_task;

    if (o_task.has_value()) {
      o_task.value()();
    } else {
      for (auto &other_worker : _m_all_workers) {
        if (_m_channel.is_closed()) break;
        if (other_worker.get() == this) continue;
        auto other_task = other_worker->steal_task();
        if (other_task.has_value()) {
          other_task.value()();
          break;
        }
      }
    }

    if (!o_task.has_value()) {
      std::this_thread::yield();
    }
    _m_heartbeat = std::chrono::steady_clock::now();

    _m_status = WORKER_STATUS_IDLE;
  }
}

inline void Worker::run() {
  std::optional<std::packaged_task<void()>> task;
  while (!_m_channel.is_closed() && _m_status == WORKER_STATUS_IDLE) {
    std::unique_lock<std::mutex> lock(_m_status_mutex);
    if (_m_paused_until > std::chrono::steady_clock::now()) {
      lock.unlock();
      std::this_thread::sleep_for(_m_paused_until -
                                  std::chrono::steady_clock::now());
      continue;
    }
    lock.unlock();

    _m_status = WORKER_STATUS_RUNNING;
    _m_channel >> task;

    if (task.has_value()) {
      task.value()();
    } else {
      std::this_thread::yield();
    }
    _m_heartbeat = std::chrono::steady_clock::now();
    _m_status = WORKER_STATUS_IDLE;
  }
}

inline std::optional<std::packaged_task<void()>> Worker::steal_task() {
  std::optional<std::packaged_task<void()>> task;
  _m_channel >> task;
  return task;
}

inline void Worker::sleep_for(std::chrono::milliseconds _m_time) {
  std::unique_lock<std::mutex> lock(_m_status_mutex);
  _m_paused_until = std::chrono::steady_clock::now() + _m_time;
}

#endif  // WORKER_H
