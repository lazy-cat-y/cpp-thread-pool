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

File: atomic-queue
*****************************************************************************/

/** @file /include/atomic-queue.h
 *  @brief Contains the AtomicQueue class.
 *  @author H. Y.
 *  @date 11/23/2024
 */

#ifndef ATOMIC_QUEUE_H
#define ATOMIC_QUEUE_H

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <utility>
#include "m-define.h"

#define ATOMIC_QUEUQ_VERSION_MASK 0xffff
#define ATOMIC_QUEUE_ALIGNMENT_OFFSET 0x10000

#define ATOMIC_NODE_ALIGNMENT_SIZE (sizeof(Node) / \
            ATOMIC_QUEUE_ALIGNMENT_OFFSET + 1) * ATOMIC_QUEUE_ALIGNMENT_OFFSET)

template <typename ValueType>
class AtomicQueue {
  struct Node {
    ValueType _value;
    std::atomic<Node *> _next;

    explicit Node() : _value{}, _next{nullptr} {}
    explicit Node(const ValueType &value) : _value{value}, _next{nullptr} {}
  };

 public:
  AtomicQueue() : _m_size{0} {
    Node *dummy = reinterpret_cast<Node *>(
        aligned_alloc(ATOMIC_QUEUE_ALIGNMENT_OFFSET, ATOMIC_NODE_ALIGNMENT_SIZE);
    assert_p(dummy != nullptr, "dummy node is nullptr");
    new (dummy) Node(ValueType{});
    _m_head.store(pack(dummy, 0));
    _m_tail.store(pack(dummy, 0));
  }

  ~AtomicQueue() {
    while (dequeue().has_value());
    Node *dummy = unpack_node(_m_head.load());
    dummy->~Node();
    free(dummy);
  }

  [[nodiscard]] std::optional<ValueType> dequeue() {
    while (true) {
      uintptr_t head = _m_head.load();
      Node *head_node = unpack_node(head);
      size_t version = unpack_version(head);

      Node *next = head_node->_next.load();
      if (next == nullptr) {
        return std::nullopt;
      }

      uintptr_t new_head = pack(next, version + 1);
      if (_m_head.compare_exchange_weak(head, new_head)) {
        ValueType value = std::move(next->_value);
        head_node->~Node();
        free(head_node);
        _m_size.fetch_sub(1);
        return value;
      }
    }
  }

  [[nodiscard]] std::optional<ValueType> front() {
    while (true) {
      uintptr_t head = _m_head.load();
      Node *head_node = unpack_node(head);
      size_t version = unpack_version(head);

      if (head_node->_next.load() == nullptr) {
        return std::nullopt;
      }

      ValueType value = head_node->_next.load()->_value;
      if (head == _m_head.load() && version == unpack_version(head)) {
        return value;
      }
    }
  }

  void enqueue(ValueType &&value) {
    Node *new_node = reinterpret_cast<Node *>(
        aligned_alloc(ATOMIC_QUEUE_ALIGNMENT_OFFSET, ATOMIC_NODE_ALIGNMENT_SIZE);
    assert_p(new_node != nullptr, "new node is nullptr");
    new (new_node) Node(ValueType{std::move(value)});
#if defined(DEBUG)
    assert_p(new_node->_next.load() == nullptr, "new node next is not null");
#endif
    while (true) {
      uintptr_t tail_pack = _m_tail.load();
      Node *tail = unpack_node(tail_pack);
      int version = unpack_version(tail_pack);
      Node *expected = nullptr;

      if (tail->_next.compare_exchange_weak(expected, new_node)) {
        uintptr_t new_tail = pack(new_node, version + 1);
        std::atomic_thread_fence(std::memory_order_release);
        _m_tail.compare_exchange_strong(tail_pack, new_tail);
        _m_size.fetch_add(1);
        std::atomic_thread_fence(std::memory_order_acquire);
#if defined(DEBUG)
        assert_p(unpack_node(_m_tail.load())->_next.load() == nullptr,
                 "tail is not the last node");
#endif
        return;
      }
    }
    assert_p(false, "enqueue failed");
  }

  void enqueue(const ValueType &value) { enqueue(ValueType{value}); }

  size_t size() const { return _m_size.load(); }

 private:
  static uintptr_t pack(Node *ptr, size_t version) {
    return (reinterpret_cast<uintptr_t>(ptr) &
            ~static_cast<uintptr_t>(ATOMIC_QUEUQ_VERSION_MASK)) |
           (version & ATOMIC_QUEUQ_VERSION_MASK);
  }

  static Node *unpack_node(uintptr_t packed) {
    return reinterpret_cast<Node *>(
        packed & ~static_cast<uintptr_t>(ATOMIC_QUEUQ_VERSION_MASK));
  }

  static size_t unpack_version(uintptr_t packed) {
    return packed & ATOMIC_QUEUQ_VERSION_MASK;
  }

  std::atomic<uintptr_t> _m_head;
  std::atomic<uintptr_t> _m_tail;
  std::atomic<size_t> _m_size;
};

#endif  // ATOMIC_QUEUE_H