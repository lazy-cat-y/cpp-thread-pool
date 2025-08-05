#ifndef LC_THREAD_POOL_H
#define LC_THREAD_POOL_H

#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>

#include "lc_config.h"
#include "lc_context.h"
#include "lc_mpmc_queue.h"
#include "lc_wait_strategy.h"

LC_NAMESPACE_BEGIN

template <size_t PoolSize, typename Meta = EmptyMetadata,
          typename WaitStrategy = AtomicWaitStrategy>
    requires std::derived_from<WaitStrategy, WaitStrategyBase>
class ThreadPool {
    using InternalTask = Context<Meta, std::function<void()>>;
public:

    ThreadPool(std::shared_ptr<MPMCQueue<InternalTask>> task_queue) {
        state_.store(State::Initializing, std::memory_order_relaxed);
        active_tasks_.store(0, std::memory_order_relaxed);
        task_queue_    = std::move(task_queue);
        wait_strategy_ = std::make_shared<WaitStrategy>();
        launch_all_workers();
        state_.store(State::Running, std::memory_order_release);
    }

    ~ThreadPool() {
        shutdown();
    }

    template <std::invocable Func>
    auto submit(Func &&func) -> std::future<std::invoke_result_t<Func>> {
        return submit(EmptyMetadata {}, std::forward<Func>(func));
    }

    template <typename Ctx, std::invocable Func>
    auto submit(Ctx &&ctx, Func &&func)
        -> std::future<std::invoke_result_t<Func>> {
        using ResultType = std::invoke_result_t<Func>;
        auto task_ptr    = std::make_shared<std::packaged_task<ResultType()>>(
            std::forward<Func>(func));

        auto future = task_ptr->get_future();

        InternalTask task {std::forward<Ctx>(ctx),
                           [task_ptr]() mutable { (*task_ptr)(); }};
        if (!task_queue_->enqueue(std::move(task))) {
            throw std::runtime_error("Failed to enqueue task");
        }

        wait_strategy_->notify();
        return future;
    }

    template <typename Func, typename... Args>
        requires std::invocable<Func, Args...>
    auto submit(Func &&func, Args &&...args)
        -> std::future<std::invoke_result_t<Func, Args...>> {
        return submit(EmptyMetadata {},
                      std::forward<Func>(func),
                      std::forward<Args>(args)...);
    }

    template <typename Ctx, typename Func, typename... Args>
        requires std::invocable<Func, Args...>
    auto submit(Ctx &&ctx, Func &&func, Args &&...args)
        -> std::future<std::invoke_result_t<Func, Args...>> {
        using ResultType = std::invoke_result_t<Func, Args...>;
        auto bound_func =
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<ResultType()>>(
            std::move(bound_func));
        auto         future = task_ptr->get_future();
        InternalTask task {std::forward<Ctx>(ctx),
                           [task_ptr]() mutable { (*task_ptr)(); }};
        if (!task_queue_->enqueue(std::move(task))) {
            throw std::runtime_error("Failed to enqueue task");
        }
        wait_strategy_->notify();
        return future;
    }

    void shutdown() {
        if (state_.load(std::memory_order_relaxed) != State::Running) {
            return;  // Already shutting down or stopped
        }
        state_.store(State::Stopping, std::memory_order_release);
        wait_strategy_->notify_all();
        for (auto &worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        state_.store(State::Stopped, std::memory_order_release);
    }

private:

    void launch_all_workers() {
        for (size_t i = 0; i < PoolSize; ++i) {
            workers_[i] = std::thread(&ThreadPool::worker_thread, this, i);
        }
    }

    void worker_thread(size_t index) {
        auto &strategy = *wait_strategy_;
        while (true) {
            InternalTask task;
            if (task_queue_->dequeue(task)) {
                strategy.reset();
                active_tasks_.fetch_add(1, std::memory_order_acq_rel);
                task.data();
                active_tasks_.fetch_sub(1, std::memory_order_acq_rel);
            } else if (state_.load(std::memory_order_relaxed) ==
                           State::Stopping &&
                       active_tasks_.load(std::memory_order_relaxed) == 0) {
                break;
            } else {
                strategy.wait();
            }
        }
    }

    enum class State {
        Initializing,
        Running,
        Stopping,
        Stopped
    };

    std::shared_ptr<MPMCQueue<InternalTask>> task_queue_;
    std::array<std::thread, PoolSize>        workers_;
    std::atomic<State>                       state_;
    std::atomic<size_t>                      active_tasks_;
    std::shared_ptr<WaitStrategy>            wait_strategy_;
};

LC_NAMESPACE_END

#endif  // LC_THREAD_POOL_H
