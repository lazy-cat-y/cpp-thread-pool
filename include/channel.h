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

/** @file /include/channel.hpp
 *  @brief Contains the Channel class.
 *  @author H. Y.
 *  @date 11/19/2024
 */

#ifndef CHANNEL_H
#define CHANNEL_H

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <optional>


template <typename ValueType>
class Segment {
 public:
  explicit Segment(size_t size)
      : _data{new ValueType[size]},
        _write_index{0},
        _read_index{0},
        _next{nullptr} {}

  ValueType *_data;
  std::atomic<int> _write_index;
  std::atomic<int> _read_index;
  std::atomic<Segment *> _next;
};

template <typename ValueType>
struct TaggedSegment {
  Segment<ValueType> *_ptr;
  std::atomic<int> _version;
};

template <typename ValueType, size_t QUEUE_SIZE = 100, size_t SEGMENT_SIZE = 10>
class Channel {
  static_assert(QUEUE_SIZE > 0, "QUEUE_SIZE must be greater than 0.");
  static_assert(SEGMENT_SIZE > 0, "SEGMENT_SIZE must be greater than 0.");
  static_assert(QUEUE_SIZE >= SEGMENT_SIZE,
                "QUEUE_SIZE must be greater than or equal to SEGMENT_SIZE.");
  static_assert(QUEUE_SIZE % SEGMENT_SIZE == 0,
                "QUEUE_SIZE must be a multiple of SEGMENT_SIZE.");
  using Segment = Segment<ValueType>;
  using TaggedSegment = TaggedSegment<ValueType>;

 public:
  Channel()
      : _m_segment_pool_size{_m_segment_count.load()},
        _m_segment_count{QUEUE_SIZE / SEGMENT_SIZE}
  // _m_queue_tail{_m_queue_head},
  // _m_queue_head{new Segment(0)},
  // _m_segment_pool_tail{_m_segment_pool_head},
  // _m_segment_pool_head{new Segment(0)}
  {
    // Segment *cur = _m_segment_pool_tail;

    // for (size_t i = 0; i < _m_segment_count.load(); ++i) {
    //   cur->_next = new Segment(SEGMENT_SIZE);
    //   cur = cur->_next;
    // }
  }

  ~Channel() {
    // Segment *cur = _m_queue_head;
    // Segment *next = nullptr;

    // while (cur != nullptr) {
    //   next = cur->_next;
    //   delete[] cur->_data;
    //   delete cur;
    //   cur = next;
    // }

    // cur = _m_segment_pool_head;

    // while (cur != nullptr) {
    //   next = cur->_next;
    //   delete[] cur->_data;
    //   delete cur;
    //   cur = next;
    // }
  }

  Channel &operator=(const Channel &) = delete;
  Channel(const Channel &) = delete;
  Channel &operator=(Channel &&) = delete;
  Channel(Channel &&) = delete;

  std::optional<ValueType> pop() {}

  template <typename T>
  void push(T &&value) {}

 private:
  // NOTE: The next pointer of the input segment should be set to nullptr.
  void push_segment_to_pool(Segment *segment) {
#if defined(DEBUG)
    assert(segment != nullptr);
    assert(segment->_next == nullptr);
#endif
    // Segment *expected_tail = _m_segment_pool_tail.load();
    // while (
    //     !_m_segment_pool_tail.compare_exchange_weak(expected_tail, segment))
    //     {
    // }
    // expected_tail->_next.store(segment);
    // _m_segment_pool_size.fetch_add(1);
  }

  Segment *pull_segment_from_pool() {
#if defined(DEBUG)
    assert(_m_segment_pool_size.load() >= 0);
#endif
    while (true) {
      // Segment *expected_head = _m_segment_pool_head->_next.load();
      // // TODO: Add a scalar to avoid problem busy wait when there is only one
      // // thread to use the channel.
      // if (expected_head == nullptr) {
      //   std::this_thread::yield();
      //   continue;
      // }

      // if (_m_segment_pool_head->_next.compare_exchange_weak(
      //         expected_head, expected_head->_next)) {
      //   _m_segment_pool_size.fetch_sub(1);
      //   expected_head->_next = nullptr;
      //   return expected_head;
      // }
    }
  }

  std::atomic<size_t> _m_segment_count;
  // head -> next is the first segment in the queue.
  // std::atomic<TaggedSegment> _m_queue_head;
  // std::atomic<TaggedSegment> _m_queue_tail;
  // head -> next is the first segment in the pool.
  // std::atomic<TaggedSegment> _m_segment_pool_head;
  // std::atomic<TaggedSegment> _m_segment_pool_tail;
  std::atomic<size_t> _m_segment_pool_size;
};

#endif  // CHANNEL_H