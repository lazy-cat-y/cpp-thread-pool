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

/**
 * For the Head and Tail pointers, we use high bits to store the pointer and
 * low bits to store the version number. The version number is used to avoid
 * the ABA problem. It is incremented every time the pointer is changed.
 *
 *  00000000 00000000 00000000 00000000 00000000 00000000 00000000
 *  ^------------------------------------------^-^---------------^
 *  |               Node Pointer               | |    version    |
 *
 * NOTE: The version number is limited to 8 bits, meaning the maximum value is
 * 255. If the version number reaches 255, it will reset to 0. This limitation
 * is acceptable because this class is designed for use with channels in the
 * worker and worker pool. In our design, worker tasks operate based on
 * channels, which use a buffer pool to store tasks and unused memory. As a
 * result, the version number is unlikely to reach 255 due to the ring size
 * constraints.
 */

namespace tp {

#define ATOMIC_QUEUQ_VERSION_MASK 0xff
#define ATOMIC_QUEUE_ALIGNMENT_OFFSET 0x100

#define ATOMIC_NODE_ALIGNMENT_SIZE (sizeof(Node) / \
            ATOMIC_QUEUE_ALIGNMENT_OFFSET + 1) * ATOMIC_QUEUE_ALIGNMENT_OFFSET)

/**
 * @brief A lock-free queue implementation using atomic operations.
 * @tparam ValueType The type of the value stored in the queue.
 */
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

  /**
   * @brief Dequeues a value from the queue.
   * @return An optional value. If the queue is empty, returns std::nullopt.
   */
  [[nodiscard]] std::optional<ValueType> dequeue() {
    uintptr_t head;
    Node *head_node;
    size_t version;
    size_t new_version;

    while (true) {
      head = _m_head.load();
      head_node = unpack_node(head);
      version = unpack_version(head);

      Node *next = head_node->_next.load();
      if (next == nullptr) {
        return std::nullopt;
      }

      // Update the version number.
      // If the version number reaches the maximum value, reset it to 0.
      new_version = version + 1;
      if (new_version > ATOMIC_QUEUQ_VERSION_MASK) {
        new_version = 0;
      }

      // Try to update the head pointer.
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

  /**
   * @brief Returns the front value of the queue without removing it.
   * @return An optional value. If the queue is empty, returns std::nullopt.
   */
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

  /**
   * @brief Enqueues a value into the queue.
   * @param value The value to enqueue. For rvalue reference.
   */
  void enqueue(ValueType &&value) {
    Node *new_node = reinterpret_cast<Node *>(aligned_alloc(
        ATOMIC_QUEUE_ALIGNMENT_OFFSET, ATOMIC_NODE_ALIGNMENT_SIZE);
#if defined(DEBUG)
    assert_p(new_node != nullptr, "new node is nullptr");
#endif
    new (new_node) Node(ValueType{std::move(value)});
    uintptr_t tail_pack = _m_tail.load();
    Node *tail = nullptr;
    int version = 0;
    Node *expected = nullptr;
    uintptr_t new_tail = 0;
    size_t new_version = 0;

#if defined(DEBUG)
    assert_p(new_node->_next.load() == nullptr, "new node next is not null");
#endif
    while (true) {
      tail_pack = _m_tail.load();
      tail = unpack_node(tail_pack);
      version = unpack_version(tail_pack);
      expected = nullptr;

      // Try to update the tail pointer.
      if (tail->_next.compare_exchange_weak(expected, new_node)) {
        new_version = version + 1;
        if (new_version > ATOMIC_QUEUQ_VERSION_MASK) {
          new_version = 0;
        }
        new_tail = pack(new_node, version + 1);

        // This is used to enforce memory order in a multi-threaded environment,
        // ensuring data consistency and visibility across threads. Together,
        // these fences implement the Release-Acquire memory model, guaranteeing
        // proper synchronization of shared data and avoiding undefined
        // behaviors caused by compiler or hardware optimizations in a
        // concurrent environment.
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

  /**
   * @brief Enqueues a value into the queue.
   * @param value The value to enqueue. For lvalue reference.
   */
  void enqueue(const ValueType &value) { enqueue(ValueType{value}); }

  /**
   * @brief Returns the end value of the queue without removing it.
   * @return An optional value. If the queue is empty, returns std::nullopt.
   */
  [[nodiscard]]
  std::optional<ValueType> end() {
    while (true) {
      uintptr_t tail = _m_tail.load();
      Node *tail_node = unpack_node(tail);
      size_t version = unpack_version(tail);

      if (tail_node->_next.load() == nullptr) {
        return std::nullopt;
      }

      ValueType value = tail_node->_value;
      if (tail == _m_tail.load() && version == unpack_version(tail)) {
        return value;
      }
    }
  }

  /**
   * @brief Returns the size of the queue.
   * @return The size of the queue.
   */
  [[nodiscard]]
  size_t size() const {
    return _m_size.load();
  }

 private:
  /**
   * @brief Packs a node pointer and a version number into a single uintptr_t
   * value.
   * @param ptr The node pointer.
   * @param version The version number.
   * @return The packed value.
   */
  static uintptr_t pack(Node *ptr, size_t version) {
    return (reinterpret_cast<uintptr_t>(ptr) &
            ~static_cast<uintptr_t>(ATOMIC_QUEUQ_VERSION_MASK)) |
           (version & ATOMIC_QUEUQ_VERSION_MASK);
  }

  /**
   * @brief Unpacks a packed value into a node pointer.
   * @param packed The packed value.
   * @return The unpacked node pointer.
   */
  static Node *unpack_node(uintptr_t packed) {
    return reinterpret_cast<Node *>(
        packed & ~static_cast<uintptr_t>(ATOMIC_QUEUQ_VERSION_MASK));
  }

  /**
   * @brief Unpacks a packed value into a version number.
   * @param packed The packed value.
   * @return The unpacked version number.
   */
  static size_t unpack_version(uintptr_t packed) {
    return packed & ATOMIC_QUEUQ_VERSION_MASK;
  }

  std::atomic<uintptr_t> _m_head;
  std::atomic<uintptr_t> _m_tail;
  std::atomic<size_t> _m_size;
};

}  // namespace tp

#endif  // ATOMIC_QUEUE_H