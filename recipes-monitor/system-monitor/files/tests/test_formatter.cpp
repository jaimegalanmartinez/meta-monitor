#include "formatter.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <string>

// Fixed timestamp: 2026-01-15T10:30:00Z = 1768473000 seconds since epoch
static monitor::Sample make_sample(double cpu_c,
                                   std::uint64_t total_kb,
                                   std::uint64_t available_kb) {
    using namespace std::chrono;
    return {
        .ts          = system_clock::time_point{seconds{1768473000}},
        .cpu_temp_c  = cpu_c,
        .mem         = {.total_kb = total_kb, .available_kb = available_kb},
    };
}

TEST_CASE("format_sample: output contains ISO-8601 timestamp prefix") {
    auto line = monitor::format_sample(make_sample(47.3, 971568, 649392));
    CHECK(line.starts_with("[2026-01-15T10:30:00Z]"));
}

TEST_CASE("format_sample: cpu temperature formatted to one decimal") {
    auto line = monitor::format_sample(make_sample(47.3, 971568, 649392));
    CHECK(line.find("cpu=47.3C") != std::string::npos);
}

TEST_CASE("format_sample: memory values and percentage are present") {
    // used = 971568 - 649392 = 322176 kB = 314.625 MiB -> "315"
    // total = 971568 kB = 948.796875 MiB -> "949"
    // pct = 100 * 322176 / 971568 = 33.16% -> "33.2"
    auto line = monitor::format_sample(make_sample(47.3, 971568, 649392));
    CHECK(line.find("mem_used=315/949 MiB") != std::string::npos);
    CHECK(line.find("(33.2%)") != std::string::npos);
}

TEST_CASE("format_sample: zero total memory does not divide by zero") {
    auto line = monitor::format_sample(make_sample(50.0, 0, 0));
    CHECK(line.find("0/0 MiB") != std::string::npos);
    CHECK(line.find("(0.0%)") != std::string::npos);
}

TEST_CASE("format_sample: available > total clamps used to zero") {
    // Defensive: kernel should never report this, but unsigned wrap would be catastrophic
    auto line = monitor::format_sample(make_sample(50.0, 100, 200));
    CHECK(line.find("mem_used=0/") != std::string::npos);
}
