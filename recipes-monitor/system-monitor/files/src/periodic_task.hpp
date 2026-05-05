#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <coroutine>
#include <exception>
#include <thread>
#include <utility>

namespace monitor {

// Minimal eager coroutine type. The body starts running on construction; the
// caller drives suspensions via `resume()` until `done()`. We deliberately
// keep the executor tiny — C++20 coroutine machinery demo, not a full runtime.
struct task {
    struct promise_type {
        std::exception_ptr error;

        task get_return_object() {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never  initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_void()         noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    using handle_t = std::coroutine_handle<promise_type>;

    explicit task(handle_t h) : h_{h} {}
    task(const task&)            = delete;
    task& operator=(const task&) = delete;
    task(task&& o) noexcept : h_{std::exchange(o.h_, handle_t{})} {}
    ~task() { if (h_) h_.destroy(); }

    bool done() const noexcept { return !h_ || h_.done(); }
    void resume() { h_.resume(); }
    void rethrow_if_failed() const {
        if (h_ && h_.promise().error) 
            std::rethrow_exception(h_.promise().error);
    }

private:
    handle_t h_;
};

// Awaiter that suspends the coroutine and blocks the calling thread for `dur`,
// checking `stop` every 100 ms so SIGTERM is observed quickly. Returning from
// await_suspend (without calling h.resume()) keeps the stack O(1) — the
// main() driver loop calls resume() each iteration at constant depth.
struct interruptible_sleep {
    std::chrono::milliseconds dur;
    const std::atomic<bool>&  stop;

    bool await_ready() const noexcept { return stop.load(std::memory_order_relaxed); }
    void await_suspend(std::coroutine_handle<>) const {
        using namespace std::chrono;
        constexpr auto tick = milliseconds{100};
        auto remaining = dur;
        while (remaining > milliseconds::zero() && !stop.load(std::memory_order_relaxed)) {
            auto slice = std::min(remaining, tick);
            std::this_thread::sleep_for(slice);
            remaining -= slice;
        }
    }
    void await_resume() const noexcept {}
};

inline interruptible_sleep sleep_for(std::chrono::milliseconds dur,
                                     const std::atomic<bool>& stop) {
    return {.dur = dur, .stop = stop};
}

} // namespace monitor
