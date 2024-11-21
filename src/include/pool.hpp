

/** @file /src/include/pool.hpp
 *  @brief Contains the Pool class.
 *  @author H. Y.
 *  @date 11/20/2024
 */

#ifndef POOL_H
#define POOL_H

#include <atomic>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
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

 private:
  std::vector<std::unique_ptr<Worker>> _m_workers;
  std::atomic<size_t> _next_worker;
  std::atomic<uint8_t> _m_status;
  std::mutex _m_status_mutex;

  void detect_dead_worker();
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
void ThreadPool<POOL_SIZE, QUEUE_SIZE, CHECK_TIME>::detect_dead_worker() {}

#endif  // POOL_H
