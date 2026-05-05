#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

namespace monitor {

struct MemInfo {
    std::uint64_t total_kb{};
    std::uint64_t available_kb{};
};

struct Sample {
    std::chrono::system_clock::time_point ts;
    double cpu_temp_c{};
    MemInfo mem{};
};

// Path overrides exist purely so host-side tests can point at fixture files.
inline constexpr const char* kThermalPath = "/sys/class/thermal/thermal_zone0/temp";
inline constexpr const char* kMeminfoPath = "/proc/meminfo";

[[nodiscard]] std::expected<double, std::error_code>
read_cpu_temp(const char* path = kThermalPath);

[[nodiscard]] std::expected<MemInfo, std::error_code>
read_meminfo(const char* path = kMeminfoPath);

} // namespace monitor
