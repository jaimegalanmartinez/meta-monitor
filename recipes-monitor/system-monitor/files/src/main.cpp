#include "formatter.hpp"
#include "periodic_task.hpp"
#include "sampler.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <expected>
#include <iostream>
#include <string>

namespace {

std::atomic<bool> shutdown_requested{false};

extern "C" void on_signal(int) noexcept {
    shutdown_requested.store(true, std::memory_order_relaxed);
}

std::expected<monitor::Sample, std::string> try_sample() {
    monitor::Sample s{.ts = std::chrono::system_clock::now()};

    auto t = monitor::read_cpu_temp();
    if (!t) return std::unexpected("read_cpu_temp failed: " + t.error().message());

    auto m = monitor::read_meminfo();
    if (!m) return std::unexpected("read_meminfo failed: " + m.error().message());

    s.cpu_temp_c = *t;
    s.mem = *m;
    return s;
}

monitor::task run_loop(std::chrono::milliseconds interval) {
    while (!shutdown_requested.load(std::memory_order_relaxed)) {
        if (auto s = try_sample(); s) {
            std::cout << monitor::format_sample(*s) << '\n';
        } else {
            std::cout << "warn: " << s.error() << '\n';
        }
        std::cout.flush();
        co_await monitor::sleep_for(interval, shutdown_requested);
    }
    co_return;
}

} // namespace

int main() {
    std::signal(SIGTERM, on_signal);
    std::signal(SIGINT, on_signal);

    auto t = run_loop(std::chrono::seconds{5});
    while (!t.done()) {
        t.resume();
    }
    t.rethrow_if_failed();
    return 0;
}
