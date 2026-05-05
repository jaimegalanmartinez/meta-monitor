SUMMARY = "C++20 system monitor (CPU temp + memory) for Raspberry Pi"
DESCRIPTION = "Periodic sampler using C++20 coroutines and C++23 std::expected. \
Reads /sys/class/thermal/thermal_zone0/temp and /proc/meminfo every 5 seconds \
and logs to stdout (captured by journald)."
HOMEPAGE = "https://github.com/jaimegalanmartinez/meta-monitor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://CMakeLists.txt \
    file://src \
    file://system-monitor.service \
"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_SERVICE:${PN} = "system-monitor.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/system-monitor.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${systemd_system_unitdir}/system-monitor.service"
