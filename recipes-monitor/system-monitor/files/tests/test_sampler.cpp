#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "sampler.hpp"

#include <fstream>
#include <string_view>
#include <unistd.h>

// RAII wrapper: creates a named temp file with given content, deletes on scope exit.
struct TempFile {
    std::string path;

    explicit TempFile(std::string_view content) {
        char tmpl[] = "/tmp/monitor_test_XXXXXX";
        int fd = ::mkstemp(tmpl);
        REQUIRE(fd != -1);
        path = tmpl;
        ::write(fd, content.data(), content.size());
        ::close(fd);
    }
    ~TempFile() { ::unlink(path.c_str()); }

    const char* c_str() const { return path.c_str(); }
};

// ── read_cpu_temp ────────────────────────────────────────────────────────────

TEST_CASE("read_cpu_temp: parses millidegrees to Celsius") {
    TempFile f{"47312\n"};
    auto result = monitor::read_cpu_temp(f.c_str());
    REQUIRE(result.has_value());
    CHECK(*result == doctest::Approx(47.312));
}

TEST_CASE("read_cpu_temp: parses value with no trailing newline") {
    TempFile f{"50000"};
    auto result = monitor::read_cpu_temp(f.c_str());
    REQUIRE(result.has_value());
    CHECK(*result == doctest::Approx(50.0));
}

TEST_CASE("read_cpu_temp: returns error for missing file") {
    auto result = monitor::read_cpu_temp("/nonexistent/thermal");
    REQUIRE(!result.has_value());
}

TEST_CASE("read_cpu_temp: returns error for non-numeric content") {
    TempFile f{"not_a_number\n"};
    auto result = monitor::read_cpu_temp(f.c_str());
    REQUIRE(!result.has_value());
    CHECK(result.error() == std::make_error_code(std::errc::invalid_argument));
}

// ── read_meminfo ─────────────────────────────────────────────────────────────

static constexpr std::string_view kMeminfoOk =
    "MemTotal:        971568 kB\n"
    "MemFree:         249168 kB\n"
    "MemAvailable:    649392 kB\n"
    "Buffers:          83456 kB\n";

TEST_CASE("read_meminfo: parses MemTotal and MemAvailable") {
    TempFile f{kMeminfoOk};
    auto result = monitor::read_meminfo(f.c_str());
    REQUIRE(result.has_value());
    CHECK(result->total_kb     == 971568);
    CHECK(result->available_kb == 649392);
}

TEST_CASE("read_meminfo: returns error for missing file") {
    auto result = monitor::read_meminfo("/nonexistent/meminfo");
    REQUIRE(!result.has_value());
}

TEST_CASE("read_meminfo: returns error when MemTotal key is absent") {
    TempFile f{"MemAvailable: 649392 kB\n"};
    auto result = monitor::read_meminfo(f.c_str());
    REQUIRE(!result.has_value());
}

TEST_CASE("read_meminfo: returns error when MemAvailable key is absent") {
    TempFile f{"MemTotal: 971568 kB\n"};
    auto result = monitor::read_meminfo(f.c_str());
    REQUIRE(!result.has_value());
}

TEST_CASE("read_meminfo: returns error for malformed value") {
    TempFile f{"MemTotal: bad kB\nMemAvailable: 649392 kB\n"};
    auto result = monitor::read_meminfo(f.c_str());
    REQUIRE(!result.has_value());
    CHECK(result.error() == std::make_error_code(std::errc::invalid_argument));
}
