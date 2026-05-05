#pragma once

#include "sampler.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <string>

namespace monitor {

inline std::string format_sample(const Sample& s) {
    auto to_mib = [](std::uint64_t kb) { return static_cast<double>(kb) / 1024.0; };

    const auto   used_kb   = s.mem.total_kb > s.mem.available_kb
                                 ? s.mem.total_kb - s.mem.available_kb
                                 : 0u;
    const double used_mib  = to_mib(used_kb);
    const double total_mib = to_mib(s.mem.total_kb);
    const double pct       = total_mib > 0.0 ? 100.0 * used_mib / total_mib : 0.0;

    return std::format(
        "[{:%FT%TZ}] cpu={:.1f}C mem_used={:.0f}/{:.0f} MiB ({:.1f}%)",
        std::chrono::floor<std::chrono::seconds>(s.ts),
        s.cpu_temp_c, used_mib, total_mib, pct);
}

} // namespace monitor
