# meta-monitor

A Yocto layer that adds a small **C++20/23 system monitor** service to a
Raspberry Pi 3B image. Designed to sit on top of
[yocto-rpi3-setup](https://github.com/jaimegalanmartinez/yocto-rpi3-setup)
(scarthgap 5.0, `raspberrypi3-64`, systemd, GCC 13).

The service samples CPU temperature (`/sys/class/thermal/thermal_zone0/temp`)
and memory (`/proc/meminfo`) every 5 seconds and writes a single line per
sample to stdout, which systemd captures into the journal:

```
[2026-05-05T12:34:56Z] cpu=47.3C mem_used=312/948 MiB (32.9%)
```

## Modern C++ highlights

The point of this project is to **demonstrate modern embedded C++** — the
domain logic is intentionally tiny so the language and platform choices stay
in the foreground.

- **C++20 coroutines** — the periodic sampler is a deliberately minimal `task`
  coroutine, written as a language showcase rather than because the problem
  requires async I/O. The interesting part is what's under the hood: a
  hand-rolled `promise_type`, a custom `interruptible_sleep` awaiter with
  chunked waking (so `SIGTERM` is observed within ~100 ms), and RAII ownership
  of the coroutine frame. A production service would use a plain loop or an
  async runtime; the coroutine exists so a reader can study the full C++20
  machinery in one small file. See `src/periodic_task.hpp`.
- **C++23 `std::expected`** — sysfs/procfs readers never throw. All I/O and
  parse errors travel through `std::expected<T, std::error_code>`, with
  `errno` mapped via `std::generic_category()`. See `src/sampler.cpp`.
- **`std::ranges` / `std::views::split`** — `/proc/meminfo` is parsed with
  range views and `std::from_chars` (no allocations per line, no streams).
- **`std::format` with chrono formatters** — sample lines are produced by a
  single `std::format` call; the timestamp uses the `{:%FT%TZ}` chrono
  formatter directly on a `time_point`.
- **systemd hardening** — the unit ships with `NoNewPrivileges`,
  `ProtectSystem=strict`, `ProtectHome`, `PrivateTmp`, and read-only
  `/sys` and `/proc` bind mounts. The binary needs nothing else.

## Layout

```
meta-monitor/
├── conf/layer.conf
├── COPYING.MIT
└── recipes-monitor/system-monitor/
    ├── system-monitor_0.1.0.bb
    └── files/
        ├── CMakeLists.txt
        ├── system-monitor.service
        └── src/{main,sampler,formatter}.{cpp,hpp}, periodic_task.hpp
```

## Adding the layer to a scarthgap build

Assuming you followed the base setup so `~/Development/yocto/poky` and
`~/Development/yocto/meta-raspberrypi` already exist:

```bash
cd ~/Development/yocto
git clone https://github.com/jaimegalanmartinez/meta-monitor.git

source poky/oe-init-build-env build
bitbake-layers add-layer ../meta-monitor

echo 'IMAGE_INSTALL:append = " system-monitor"' >> conf/local.conf

bitbake core-image-base
```

Flash the resulting `tmp/deploy/images/raspberrypi3-64/core-image-base-*.wic.bz2`
to an SD card and boot the Pi.

## Verifying on the device

```bash
systemctl status system-monitor      # active (running)
journalctl -u system-monitor -f      # one line every 5 seconds
systemctl stop system-monitor        # exits cleanly within ~1 s (SIGTERM)
```

## Building the C++ app standalone (no Yocto)

The application also builds on any host with GCC 13+ and CMake 3.20+. Useful
for development and exercised by CI:

```bash
cmake -S recipes-monitor/system-monitor/files -B build
cmake --build build
./build/system-monitor       # Ctrl-C to stop
```

## License

MIT — see `COPYING.MIT`.
