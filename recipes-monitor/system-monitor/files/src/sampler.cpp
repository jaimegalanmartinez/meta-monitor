#include "sampler.hpp"

#include <charconv>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

namespace monitor {

namespace {

std::expected<std::string, std::error_code> slurp(const char* path) {
    std::ifstream in{path};
    if (!in) return std::unexpected(std::error_code{errno, std::generic_category()});
    std::ostringstream ss;
    ss << in.rdbuf();
    if (in.bad()) return std::unexpected(std::error_code{errno, std::generic_category()});
    return ss.str();
}

std::expected<std::uint64_t, std::error_code>
parse_meminfo_kb(std::string_view contents, std::string_view key) {
    // views::split produces sub-ranges, not string_views; this converts one.
    auto to_sv = [](auto range) -> std::string_view {
        return {&*range.begin(), static_cast<std::size_t>(std::ranges::distance(range))};
    };

    for (auto line_range : contents | std::views::split('\n')) {
        std::string_view line = to_sv(line_range);
        if (!line.starts_with(key)) 
            continue;
        auto colon = line.find(':');
        if (colon == std::string_view::npos)
            continue;
        auto rest = line.substr(colon + 1);

        while (rest.starts_with(' ')) 
        rest.remove_prefix(1);
        std::uint64_t value = 0;
        auto [ptr, ec] = std::from_chars(rest.data(), rest.data() + rest.size(), value);
        if (ec != std::errc{}) {
            return std::unexpected(std::make_error_code(std::errc::invalid_argument));
        }
        return value;
    }
    return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
}

} // namespace

std::expected<double, std::error_code> read_cpu_temp(const char* path) {
    auto contents = slurp(path);
    if (!contents) 
        return std::unexpected(contents.error());

    long long millideg = 0;
    auto [ptr, ec] = std::from_chars(contents->data(),
                                     contents->data() + contents->size(),
                                     millideg);
    if (ec != std::errc{}) {
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }
    return static_cast<double>(millideg) / 1000.0;
}

std::expected<MemInfo, std::error_code> read_meminfo(const char* path) {
    auto contents = slurp(path);
    if (!contents)
        return std::unexpected(contents.error());

    auto total = parse_meminfo_kb(*contents, "MemTotal");
    if (!total) 
        return std::unexpected(total.error());

    auto avail = parse_meminfo_kb(*contents, "MemAvailable");
    if (!avail)
        return std::unexpected(avail.error());

    return MemInfo{*total, *avail};
}

} // namespace monitor
