

/** @file /src/include/pool.hpp
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

template <size_t POOL_SIZE = 10, size_t QUEUE_SIZE = 100, size_t CHECK_TIME = 5>
class ThreadPool {
 public:
  ThreadPool() : _next_worker(0), _m_status(THREAD_POOL_STATUS_IDLE) {
    for (size_t i = 0; i < POOL_SIZE; ++i) {
      _m_workers.push_back(std::make_unique<Worker>());
    }
    for (auto &worker : _m_workers) {
      worker->start_pool(_m_workers);
    }
    _m_status.exchange(THREAD_POOL_STATUS_RUNNING);
  }

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;

  ~ThreadPool() {
    _m_status.exchange(THREAD_POOL_STATUS_STOPPING);
    for (auto &worker : _m_workers) {
      worker->stop();
      worker->join();
    }
    _m_status.exchange(THREAD_POOL_STATUS_STOPPED);
  }

  template <typename Func, typename... Args>
  std::future<typename std::invoke_result_t<Func, Args...>> submit(
      Func &&func, Args &&...args);

  void shutdown();

  int status() const { return _m_status; }

#if !defined (ENABLE_TEST)
 private:
#endif
  void submit_monitor_taskers();
  void restart_worker(size_t worker_id);

  std::vector<std::unique_ptr<Worker>> _m_workers;
  std::atomic<size_t> _next_worker;
  std::atomic<uint8_t> _m_status;
  std::mutex _m_status_mutex;

  std::unique_ptr<Worker> _m_dead_worker;
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
