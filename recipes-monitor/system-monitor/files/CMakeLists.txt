cmake_minimum_required(VERSION 3.20)
project(system-monitor CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── application ──────────────────────────────────────────────────────────────
add_executable(system-monitor
    src/main.cpp
    src/sampler.cpp
)

target_compile_options(system-monitor PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)

include(GNUInstallDirs)
install(TARGETS system-monitor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ── tests ─────────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

enable_testing()

add_executable(tests
    tests/test_sampler.cpp
    tests/test_formatter.cpp
    src/sampler.cpp
)

target_include_directories(tests PRIVATE src)
target_link_libraries(tests PRIVATE doctest::doctest)
target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(NOT CMAKE_CROSSCOMPILING)
    include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
    doctest_discover_tests(tests)
endif()
