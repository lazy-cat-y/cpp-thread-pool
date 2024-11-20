

/** @file /src/include/channel.hpp
 *  @brief Contains the Channel class.
 *  @author H. Y.
 *  @date 11/19/2024
 */

#ifndef CHANNEL_H
#define CHANNEL_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>

template <typename ValueType, size_t MAX_SIZE>
class Channel {
 public:
  Channel() : _m_closed(false) {}

  Channel(const Channel &) = delete;
  Channel &operator=(const Channel &) = delete;
  Channel(Channel &&) = delete;
  Channel &operator=(Channel &&) = delete;

  ~Channel() = default;

  bool is_closed() {
    std::lock_guard<std::mutex> lock(_m_mutex);
    return _m_closed;
  }

  size_t size() {
    std::lock_guard<std::mutex> lock(_m_mutex);
    if (_m_closed) return -1;
    return _m_queue.size();
  }

  void send(std::optional<ValueType> value);

  std::optional<ValueType> receive();

  void close();

  Channel &operator<<(std::optional<ValueType> &&value) {
    send(std::move(value));
    return *this;
  }

  Channel &operator>>(std::optional<ValueType> &value) {
    value = receive();
    return *this;
  }

  std::optional<ValueType> operator()() { return receive(); }

 private:
  std::queue<ValueType, std::deque<ValueType>> _m_queue;
  std::mutex _m_mutex;
  std::condition_variable _m_condition;
  bool _m_closed;
};

template <typename ValueType, size_t MAX_SIZE>
void Channel<ValueType, MAX_SIZE>::send(std::optional<ValueType> value) {
  std::unique_lock<std::mutex> lock(_m_mutex);
  if (_m_closed) {
    throw std::runtime_error("Cannot send to a closed channel");
  }

  _m_condition.wait(lock,
                    [this] { return _m_queue.size() < MAX_SIZE || _m_closed; });

  if (_m_closed) {
    throw std::runtime_error("Cannot send to a closed channel");
  }

  if (!value.has_value()) {
    throw std::runtime_error("Cannot send an empty value");
  }
  _m_queue.push(std::move(value.value()));
  _m_condition.notify_one();
}

template <typename ValueType, size_t MAX_SIZE>
std::optional<ValueType> Channel<ValueType, MAX_SIZE>::receive() {
  std::unique_lock<std::mutex> lock(_m_mutex);

  _m_condition.wait(lock, [this] { return !_m_queue.empty() || _m_closed; });

  if (_m_queue.empty() && _m_closed) {
    return std::nullopt;
  }

  ValueType value = std::move(_m_queue.front());
  _m_queue.pop();
  _m_condition.notify_one();
  return std::optional<ValueType>(std::move(value));
}

template <typename ValueType, size_t MAX_SIZE>
void Channel<ValueType, MAX_SIZE>::close() {
  std::unique_lock<std::mutex> lock(_m_mutex);
  _m_closed = true;
  _m_condition.notify_all();

  while (!_m_queue.empty()) {
    _m_queue.front().~ValueType();
    _m_queue.pop();
  }
}

#endif  // CHANNEL_H