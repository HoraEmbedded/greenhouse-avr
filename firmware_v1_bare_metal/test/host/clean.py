"""Cross-platform 'make clean' / pre-coverage cleanup for test/host.

`rm -f` isn't available on a bare MinGW.org toolchain (no coreutils
included, unlike Linux/macOS or MSYS2) -- this replaces it with something
that works identically everywhere Python runs, which is everywhere this
project already needs Python anyway.

Never errors on a missing file (matching `rm -f`'s behaviour), and
handles both extension-less binaries (Linux/macOS) and .exe (Windows).

Run: python clean.py
"""
import glob
import os

TEST_NAMES = [
    "test_hysteresis", "test_thresholds", "test_command", "test_fault_handling",
    "test_soil", "test_soil_calibrated_normal", "test_soil_calibrated_inverted",
    "test_dht22_decode", "test_ring_buffer", "test_water_level",
    "test_rtc_decode", "test_schedule",
]

ARTIFACT_PATTERNS = ["*.gcda", "*.gcno", "*.gcov", "cov_*"]


def remove_if_exists(path: str) -> None:
    if os.path.isfile(path):
        os.remove(path)


def main() -> None:
    removed = []

    for name in TEST_NAMES:
        for candidate in (name, name + ".exe"):
            if os.path.isfile(candidate):
                os.remove(candidate)
                removed.append(candidate)

    for pattern in ARTIFACT_PATTERNS:
        for f in glob.glob(pattern):
            if os.path.isfile(f):
                os.remove(f)
                removed.append(f)

    for f in removed:
        print(f"  removed {f}")


if __name__ == "__main__":
    main()
