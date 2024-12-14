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
#include <limits>
#include <optional>
#include <utility>
#include "def.h"

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
 * is acceptable because this class is designed for using with channels in the
 * worker and worker pool. In our design, worker tasks operate based on
 * channels, which use a buffer pool to store tasks and unused memory. As a
 * result, the version number is unlikely to reach 255 due to the ring size
 * constraints.
 */

namespace tp {

template <typename PointerType>
class VersionPointer {
  struct alignas(16) Pointer {
#if !defined(USE_LIBATOMIC)
    void *_m_ptr;
#else
    PointerType *_m_ptr;
#endif
    uint64_t _m_version;

    bool operator==(const Pointer &_m_other) const {
      return _m_ptr == _m_other._m_ptr && _m_version == _m_other._m_version;
    }
  };

  static_assert(sizeof(Pointer) == 16, "Pointer is not 16 bytes");
  static_assert(alignof(Pointer) >= 16, "Pointer is not 16-byte aligned");

 public:
  explicit VersionPointer() : _m_data({nullptr, 0}) {}

  explicit VersionPointer(PointerType *_m_ptr) : _m_data({_m_ptr, 0}) {}

  VersionPointer(const VersionPointer &other) = delete;
  VersionPointer &operator=(const VersionPointer &other) = delete;
  VersionPointer(VersionPointer &&other) = delete;
  VersionPointer &operator=(VersionPointer &&other) = delete;

  ~VersionPointer() = default;

  bool update(PointerType *_m_expected_pointer, PointerType *_m_new_pointer) {
#if !defined(USE_LIBATOMIC)
    Pointer _m_expected;
    Pointer _m_desired;

    _m_expected._m_ptr = reinterpret_cast<void *>(_m_expected_pointer);
    _m_expected._m_version = 0;

    while (true) {
      Pointer _m_current = load_current();

      if (_m_current._m_ptr != _m_expected._m_ptr) {
        return false;
      }

      _m_desired._m_ptr = reinterpret_cast<void *>(_m_new_pointer);
      _m_desired._m_version = _m_current._m_version + 1;

      if (compare_exchange_16_bytes(_m_expected, _m_desired)) {
        return true;
      }

      _m_expected = _m_current;
    }

#else
    Pointer _m_expected = _m_data.load(std::memory_order_acquire);

    while (true) {
      if (_m_expected._m_ptr != _m_expected_pointer) {
        return false;
      }

      Pointer _m_new = {_m_new_pointer, _m_expected._m_version + 1};

      if (_m_data.compare_exchange_weak(_m_expected, _m_new,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire)) {
        return true;
      }
    }
#endif
  }

  PointerType *get_pointer() const {
    return _m_data.load(std::memory_order_acquire)._m_ptr;
  }

  uint64_t get_version() const {
    return _m_data.load(std::memory_order_acquire)._m_version;
  }

 protected:
 private:
#if !defined(USE_LIBATOMIC)
  bool compare_exchange_16_bytes(Pointer &_m_expected, Pointer &_m_desired) {
#if defined(__x86_64__) || defined(__i386__)
    bool _m_result = false;

    __asm__ __volatile__("lock cmpxchg16b %1"
                         : "=@ccz"(_m_result), "+m"(this->_m_data),
                           "+a"(_m_expected._m_ptr),
                           "+d"(_m_expected._m_version)
                         : "b"(_m_desired._m_ptr), "c"(_m_desired._m_version)
                         : "memory");
    return _m_result;
// arm
#elif defined(__aarch64__)
    uint64_t _m_expected_ptr = reinterpret_cast<uint64_t>(_m_expected._m_ptr);
    uint64_t _m_expected_version = _m_expected._m_version;
    uint64_t _m_desired_ptr = reinterpret_cast<uint64_t>(_m_desired._m_ptr);
    uint64_t _m_desired_version = _m_desired._m_version;
    uint64_t _m_result = 0;

    asm volatile(
        "ldxp x0, x1, [%2]\n"       // Load the current value from _m_data
        "cmp x0, %3\n"              // Compare the pointer part
        "cset %w0, eq\n"            // Set result to 1 if equal
        "cmp x1, %4\n"              // Compare the version part
        "csetm %w0, eq\n"           // Update result only if both match
        "b.ne 1f\n"                 // If not equal, branch to end
        "stxp %w0, %5, %6, [%2]\n"  // Attempt to store desired value
        "cbnz %w0, 1f\n"            // If store failed, branch to end
        "1:\n"
        : "=&r"(_m_result)           // Output
        : "r"(this), "r"(&_m_data),  // Inputs
          "r"(_m_expected_ptr), "r"(_m_expected_version), "r"(_m_desired_ptr),
          "r"(_m_desired_version)
        : "x0", "x1", "cc", "memory"  // Clobbered registers
    );

    if (_m_result == 0) {
      _m_expected._m_ptr = reinterpret_cast<PointerType *>(_m_desired._m_ptr);
      _m_expected._m_version = _m_desired._m_version;
      return true;
    }

    asm volatile("ldxp %0, %1, [%2]"
                 : "=r"(_m_expected._m_ptr), "=r"(_m_expected._m_version)
                 : "r"(&_m_data)
                 : "memory");
    return false;
#else
#error "Unsupported platform for load_current"
#endif
  };

  Pointer load_current() {
#if defined(__x86_64__) || defined(__i386__)
    Pointer _m_current = _m_data; 
    return _m_current;
#elif defined(__aarch64__)
    Pointer _m_current;
    asm volatile("ldxp %0, %1, [%2]"
                 : "=r"(_m_current._m_ptr), "=r"(_m_current._m_version)
                 : "r"(&_m_data)
                 : "memory");
    return _m_current;
#else
#error "Unsupported platform for load_current"
#endif
  }

  alignas(16) Pointer _m_data;
#else

  std::atomic<Pointer> _m_data __attribute__((aligned(16)));

#endif
};

template <typename DataType>
class AtomicQueue {
 public:
  explicit AtomicQueue() : _m_head(nullptr), _m_tail(nullptr), _m_size(0) {}

  AtomicQueue(const AtomicQueue &other) = delete;
  AtomicQueue &operator=(const AtomicQueue &other) = delete;
  AtomicQueue(AtomicQueue &&other) = delete;
  AtomicQueue &operator=(AtomicQueue &&other) = delete;

  // TODO: Implement the destructor
  ~AtomicQueue() = default;

  bool push(DataType &&data) {}

  bool push(const DataType &data) {}

  std::optional<DataType> pop() {}

  DataType front() const {}

  DataType back() const {}

  bool empty() const {}

  size_t size() const {}

  void clear() {}

 protected:
 private:
  VersionPointer<DataType> _m_head;
  VersionPointer<DataType> _m_tail;
  std::atomic<size_t> _m_size;
};

}  // namespace tp

#endif  // ATOMIC_QUEUE_H