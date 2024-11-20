

/** @file /src/include/work.hpp
 *  @brief Contains the Worker class.
 *  @author H. Y.
 *  @date 11/19/2024
 */

#ifndef WORKER_H
#define WORKER_H

#include <cstdint>
#include <future>
#include <optional>
#include <thread>
#include <type_traits>
#include "channel.hpp"

#define WORKER_STATUS_IDLE 0
#define WORKER_STATUS_RUNNING 1
#define WORKER_STATUS_STOPPING 2
#define WORKER_STATUS_STOPPED 3

class Worker {
 public:
  Worker() : _m_status(WORKER_STATUS_IDLE) {}

  Worker(const Worker &) = delete;
  Worker &operator=(const Worker &) = delete;
  Worker(Worker &&) = delete;
  Worker &operator=(Worker &&) = delete;

  ~Worker() {
      stop();
      join();
  }

  inline void start();

  inline void stop();

  inline void join();

  inline void run();

  template <typename Func, typename... Args>
  std::future<typename std::invoke_result_t<Func, Args...>> submit(
      Func &&func, Args &&...args);

 private:
  std::thread _m_thread;
  uint8_t _m_status;
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

inline void Worker::start() {
  if (_m_status == WORKER_STATUS_STOPPED || _m_status == WORKER_STATUS_STOPPING) {
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
  _m_status = WORKER_STATUS_STOPPED;
}

#endif  // WORKER_H
