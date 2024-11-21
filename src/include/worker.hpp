

/** @file /src/include/work.hpp
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

class Worker {
 public:
  Worker()
      : _m_status{WORKER_STATUS_IDLE},
        _m_paused_until{std::chrono::steady_clock::now()} {}

  Worker(const Worker &) = delete;
  Worker &operator=(const Worker &) = delete;
  Worker(Worker &&) = delete;
  Worker &operator=(Worker &&) = delete;

  ~Worker() {
    stop();
    join();
  }

  inline void start();

  inline void start_pool(std::vector<std::unique_ptr<Worker>> &_m_all_workers);

  inline void stop();

  inline void join();

  inline void run();

  inline void run_pool(std::vector<std::unique_ptr<Worker>> &workers);

  template <typename Func, typename... Args>
  std::future<typename std::invoke_result_t<Func, Args...>> submit(
      Func &&func, Args &&...args);

  inline std::optional<std::packaged_task<void()>> steal_task();

  inline int status() const { return _m_status; }

  const Worker *get() const { return this; }

  inline bool is_idle() const { return _m_status == WORKER_STATUS_IDLE; }

  inline void sleep_for(std::chrono::milliseconds _m_time);

 private:
  std::thread _m_thread;
  uint8_t _m_status;
  std::mutex _m_status_mutex;
  std::chrono::steady_clock::time_point _m_paused_until;
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
  if (_m_status == WORKER_STATUS_RUNNING) return;

  _m_status = WORKER_STATUS_RUNNING;
  _m_thread = std::thread(&Worker::run_pool, this, std::ref(_m_all_workers));
}

inline void Worker::start() {
  if (_m_status == WORKER_STATUS_STOPPED ||
      _m_status == WORKER_STATUS_STOPPING) {
    throw std::runtime_error("Worker is stopping or stopped.");
  }
  if (_m_status == WORKER_STATUS_RUNNING) return;

  _m_status = WORKER_STATUS_RUNNING;
  _m_thread = std::thread(&Worker::run, this);
}

inline void Worker::stop() {
  if (_m_status == WORKER_STATUS_STOPPED) return;

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
  while (!_m_channel.is_closed() && _m_status == WORKER_STATUS_RUNNING) {
    std::unique_lock<std::mutex> lock(_m_status_mutex);
    if (_m_paused_until > std::chrono::steady_clock::now()) {
      lock.unlock();
      std::this_thread::sleep_for(_m_paused_until -
                                  std::chrono::steady_clock::now());
      continue;
    }
    lock.unlock();

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
  }
}

inline void Worker::run() {
  std::optional<std::packaged_task<void()>> task;
  while (!_m_channel.is_closed() && _m_status == WORKER_STATUS_RUNNING) {
    _m_channel >> task;

    if (task.has_value()) {
      task.value()();
    } else {
      break;
    }
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
