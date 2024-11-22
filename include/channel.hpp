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

File: channel
*****************************************************************************/


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

/**
 * @brief A thread-safe channel implementation for exchanging data between
 * threads.
 * @tparam ValueType The type of data stored in the channel.
 * @tparam MAX_SIZE The maximum size of the channel.
 */
template <typename ValueType, size_t MAX_SIZE>
class Channel {
 public:
  /**
   * @brief Constructs a new Channel object.
   */
  Channel() : _m_closed(false) {}

  /* Delete copy constructor and copy assignment operator */
  Channel(const Channel &) = delete;
  Channel &operator=(const Channel &) = delete;
  Channel(Channel &&) = delete;
  Channel &operator=(Channel &&) = delete;

  /**
   * @brief Destroys the Channel object.
   */
  ~Channel() = default;

  /**
   * @brief Checks if the channel is closed.
   * @return True if the channel is closed, false otherwise.
   */
  bool is_closed() {
    std::lock_guard<std::mutex> lock(_m_mutex);
    return _m_closed;
  }

  /**
   * @brief Gets the current size of the channel.
   * @return The size of the channel, or -1 if the channel is closed.
   */
  size_t size() {
    std::lock_guard<std::mutex> lock(_m_mutex);
    return _m_queue.size();
  }

  /**
   * @brief Sends a value to the channel.
   * @param value The value to send, wrapped in std::optional.
   * @throw std::runtime_error if the channel is closed or the value is empty.
   */
  void send(std::optional<ValueType> value);

  /**
   * @brief Receives a value from the channel.
   * @return An optional containing the received value, or std::nullopt if the
   * channel is closed.
   */
  std::optional<ValueType> receive();

  /**
   * @brief Closes the channel, preventing further sends.
   */
  void close();

  /**
   * @brief Overloads the << operator to send a value to the channel.
   * @param value The value to send, wrapped in std::optional.
   * @return A reference to the Channel object.
   */
  Channel &operator<<(std::optional<ValueType> &&value) {
    send(std::move(value));
    return *this;
  }

  /**
   * @brief Overloads the >> operator to receive a value from the channel.
   * @param value A reference to an optional to store the received value.
   * @return A reference to the Channel object.
   */
  Channel &operator>>(std::optional<ValueType> &value) {
    value = receive();
    return *this;
  }

  /**
   * @brief Overloads the function call operator to receive a value from the
   * channel.
   * @return An optional containing the received value, or std::nullopt if the
   * channel is closed.
   */
  std::optional<ValueType> operator()() { return receive(); }

 private:
  /**
   * @brief The queue storing the values in the channel.
   */
  std::queue<ValueType, std::deque<ValueType>> _m_queue;

  /**
   * @brief A mutex for synchronizing access to the channel.
   */
  std::mutex _m_mutex;

  /**
   * @brief A condition variable for thread synchronization.
   */
  std::condition_variable _m_condition;

  /**
   * @brief A flag indicating whether the channel is closed.
   */
  bool _m_closed;
};

/**
 * @brief Sends a value to the channel.
 * @param value The value to send, wrapped in std::optional.
 * @throw std::runtime_error if the channel is closed or the value is empty.
 */
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

/**
 * @brief Receives a value from the channel.
 * @return An optional containing the received value, or std::nullopt if the
 * channel is closed.
 */
template <typename ValueType, size_t MAX_SIZE>
std::optional<ValueType> Channel<ValueType, MAX_SIZE>::receive() {
  std::unique_lock<std::mutex> lock(_m_mutex);

  _m_condition.wait(lock, [this] { return !_m_queue.empty() || _m_closed; });

  if (_m_queue.empty()) {
    return std::nullopt;
  }

  ValueType value = std::move(_m_queue.front());
  _m_queue.pop();
  _m_condition.notify_one();
  return std::optional<ValueType>(std::move(value));
}

/**
 * @brief Closes the channel, preventing further sends.
 */
template <typename ValueType, size_t MAX_SIZE>
void Channel<ValueType, MAX_SIZE>::close() {
  std::unique_lock<std::mutex> lock(_m_mutex);
  _m_closed = true;
  _m_condition.notify_all();
}

#endif  // CHANNEL_H