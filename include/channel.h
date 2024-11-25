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
#include "atomic-queue.h"

#if CPP_CURRENT_VERSION >= CPP_VERSION_17
#include <memory_resource>
#define ALLOCATOR(T) std::pmr::polymorphic_allocator<T>
#else
#include <memory>
#define ALLOCATOR(T) std::allocator<T>
#endif


template <typename ValueType>
class Segment {
 public:
  explicit Segment(size_t size)
      : _allocator{},
        _data(_allocator.allocate(size)),
        _write_index{0},
        _read_index{0},
        _next{0} {
    for (size_t i = 0; i < size; ++i) {
      _data[i] = ValueType{};
    }
  }

  ~Segment() { _allocator.deallocate(_data, _write_index.load()); }

  ALLOCATOR(ValueType) _allocator;
  ValueType *_data;
  std::atomic<int> _write_index;
  std::atomic<int> _read_index;
  std::atomic<size_t> _next;
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
  // using TaggedSegment = TaggedSegment<ValueType>;

 public:
  Channel()
      : _m_segment_allocator{},
        _m_segment_pool_size{_m_segment_count.load()},
        _m_segment_count{QUEUE_SIZE / SEGMENT_SIZE} {
    _m_local_memory_stack = _m_segment_allocator.allocate(_m_segment_count);
    for (size_t i = 0; i < _m_segment_count; ++i) {
      new (_m_local_memory_stack + i) Segment(SEGMENT_SIZE);
    }
    for (size_t i = 0; i < _m_segment_count; ++i) {
      push_segment_to_pool(i);
    }
  }

  ~Channel() {
    for (size_t i = 0; i < _m_segment_count; ++i) {
      _m_local_memory_stack[i].~Segment();
    }
    _m_segment_allocator.deallocate(_m_local_memory_stack, _m_segment_count);
  }

  Channel &operator=(const Channel &) = delete;
  Channel(const Channel &) = delete;
  Channel &operator=(Channel &&) = delete;
  Channel(Channel &&) = delete;

  // NOTE: We store the offset of the segment in the queue, so we need to make
  // sure that all operations are under the same memory order(or we can say that
  // whole operations are atomic). Therefore we must use memory_order operations
  // and spin the loop until the operation is successful. But!! We need to set a
  // limit of spinning times to avoid deadlocks.
  std::optional<ValueType> reverse() {}

  bool submit(const ValueType &&value) {}

  bool submit(const ValueType &value) {
    submit(ValueType{value});
  }

 private:
  // NOTE: The next pointer of the input segment should be set to nullptr.
  void push_segment_to_pool(size_t _offset) {
#if defined(DEBUG)
#endif
  }

  Segment *pull_segment_from_pool() {
#if defined(DEBUG)
    assert(_m_segment_pool_size.load() >= 0);
#endif
    while (true) {
    }
  }

  std::atomic<size_t> _m_segment_count;

  // Save the offset of the first segment in the local memory stack.
  tp::AtomicQueue<size_t> _m_queue;
  tp::AtomicQueue<size_t> _m_segment_pool;
  // local memory stack for channel.
  Segment *_m_local_memory_stack;
  ALLOCATOR(Segment) _m_segment_allocator;

  std::atomic<size_t> _m_segment_pool_size;
};

#endif  // CHANNEL_H