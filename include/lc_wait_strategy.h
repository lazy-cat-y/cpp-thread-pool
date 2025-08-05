

#ifndef LC_WAIT_STRATEGY_H
#define LC_WAIT_STRATEGY_H

#include <sys/types.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>

#include "lc_config.h"

LC_NAMESPACE_BEGIN

class WaitStrategyBase {
public:
    virtual ~WaitStrategyBase() = default;
    virtual void wait()         = 0;  // Wait for a condition to be met
    virtual void notify()       = 0;  // Notify waiting threads
    virtual void notify_all()   = 0;  // Notify all waiting threads
    virtual void reset()        = 0;  // Reset the wait strategy, if applicable
};

template <uint64_t Timeout = 10>
class PassiveWaitStrategy : public WaitStrategyBase {
public:

    PassiveWaitStrategy() {}

    void wait() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(Timeout));
    }

    void notify() override {}

    void notify_all() override {}

    void reset() override {}
};

template <size_t KSpinCount = 64, size_t KPauseCount = 64>
class SpinBackOffWaitStrategy : public WaitStrategyBase {

public:
    SpinBackOffWaitStrategy() : spin_count_(0) {}

    void wait() override {
        if (spin_count_ < KSpinCount) {
            ++spin_count_;
        } else if (spin_count_ < KSpinCount + KPauseCount) {
            ++spin_count_;
#if defined(__x86_64__) || defined(_M_X64)
            _mm_pause();  // Pause instruction for x86/x64
#elif defined(__aarch64__)
            asm volatile("yield" ::: "memory");  // Yield instruction for ARM64
#else
            std::this_thread::yield();  // Yield for other architectures
#endif
        }
    }

    void notify() override {}

    void notify_all() override {}

    void reset() override {
        spin_count_ = 0;
    }

private:
    uint64_t spin_count_;
};

class AtomicWaitStrategy : public WaitStrategyBase {
public:
    AtomicWaitStrategy() {
        notified_.store(false, std::memory_order_relaxed);
    }

    void wait() override {
        notified_.wait(false, std::memory_order_acquire);
    }

    void notify() override {
        notified_.store(true, std::memory_order_release);
        notified_.notify_one();
    }

    void notify_all() override {
        notified_.store(true, std::memory_order_release);
        notified_.notify_all();
    }

    void reset() override {
        notified_.store(false, std::memory_order_relaxed);
    }

private:
    std::atomic<bool> notified_;
};

class ConditionVariableWaitStrategy : public WaitStrategyBase {
public:
    ConditionVariableWaitStrategy() : notified_(false) {}

    void wait() override {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return notified_; });
    }

    void notify() override {
        std::scoped_lock<std::mutex> lock(mtx_);
        notified_ = true;
        cv_.notify_one();
    }

    void notify_all() override {
        std::scoped_lock<std::mutex> lock(mtx_);
        notified_ = true;
        cv_.notify_all();
    }

    void reset() override {
        std::scoped_lock<std::mutex> lock(mtx_);
        notified_ = false;
    }

private:
    std::condition_variable cv_;
    std::mutex              mtx_;
    bool                    notified_;
};

LC_NAMESPACE_END

#endif  // LC_WAIT_STRATEGY_H
