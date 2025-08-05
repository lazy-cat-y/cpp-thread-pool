#ifndef LC_MPMC_QUEUE_H
#define LC_MPMC_QUEUE_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "lc_config.h"

LC_NAMESPACE_BEGIN

template <typename Tp_>
    requires std::is_move_constructible_v<Tp_>
class MPMCQueue {
    struct alignas(64) Cell {
        std::atomic<std::size_t> sequence;
        Tp_                      value;
    };

    static constexpr size_t __LC_CACHE_LINE_SIZE = 64;
    typedef char            __lc_cacheline_pad_t[__LC_CACHE_LINE_SIZE];

public:

    explicit MPMCQueue(std::size_t queue_size) :
        pool_mask_(queue_size - 1),
        pool_(std::make_unique<Cell[]>(queue_size)) {
        if (queue_size < 2 || (queue_size & pool_mask_) != 0) {
            throw std::invalid_argument("Queue size must be a power of two.");
        }
        for (std::size_t i = 0; i < queue_size; ++i) {
            pool_[i].sequence.store(i, std::memory_order_relaxed);
        }
        enqueue_index_.store(0, std::memory_order_relaxed);
        dequeue_index_.store(0, std::memory_order_relaxed);
    }

    ~MPMCQueue() = default;

    MPMCQueue()                             = delete;
    MPMCQueue(const MPMCQueue &)            = delete;
    MPMCQueue &operator=(const MPMCQueue &) = delete;
    MPMCQueue(MPMCQueue &&)                 = delete;
    MPMCQueue &operator=(MPMCQueue &&)      = delete;

    [[nodiscard]] bool enqueue(Tp_ value) {
        std::size_t pos = enqueue_index_.load(std::memory_order_relaxed);
        while (true) {
            Cell         &cell = pool_[pos & pool_mask_];
            std::size_t   seq  = cell.sequence.load(std::memory_order_acquire);
            std::intptr_t diff = (std::intptr_t)seq - (std::intptr_t)pos;
            if (diff == 0) {
                if (enqueue_index_.compare_exchange_weak(
                        pos,
                        pos + 1,
                        std::memory_order_relaxed)) {
                    cell.value = std::move(value);
                    cell.sequence.store(pos + 1, std::memory_order_release);
                    return true;  // Successfully enqueued
                }
            } else if (diff < 0) {
                return false;     // Queue is full
            } else {
                pos = enqueue_index_.load(std::memory_order_relaxed);
            }
        }
        LC_ASSERT(false, "Should never reach here");
        return false;  // Should never reach here
    }

    [[nodiscard]] bool dequeue(Tp_ &value) {
        std::size_t pos = dequeue_index_.load(std::memory_order_relaxed);
        while (true) {
            Cell         &cell = pool_[pos & pool_mask_];
            std::size_t   seq  = cell.sequence.load(std::memory_order_acquire);
            std::intptr_t diff = (std::intptr_t)seq - (std::intptr_t)(pos + 1);
            if (diff == 0) {
                if (dequeue_index_.compare_exchange_weak(
                        pos,
                        pos + 1,
                        std::memory_order_relaxed)) {
                    value = std::move(cell.value);
                    cell.sequence.store(pos + pool_mask_ + 1,
                                        std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false;  // Queue is empty
            } else {
                pos = dequeue_index_.load(std::memory_order_relaxed);
            }
        }
        LC_ASSERT(false, "Should never reach here");
        return false;  // Should never reach here
    }

private:

    std::unique_ptr<Cell[]> pool_;
    const std::size_t       pool_mask_;
    alignas(64) std::atomic<std::size_t> enqueue_index_;
    alignas(64) std::atomic<std::size_t> dequeue_index_;
};

LC_NAMESPACE_END

#endif  // LC_MPMC_QUEUE_H
