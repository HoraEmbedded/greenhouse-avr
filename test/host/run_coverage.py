"""Cross-platform replacement for the coverage recipe that used to live
directly in the Makefile as shell commands (`rm -f *.gcda`, a bash `for`
loop). Those work fine under a real Unix shell, but mingw32-make on
Windows spawns each recipe line directly via CreateProcess when it can't
find sh.exe -- no wildcard expansion, no `for ... do ... done` syntax.
Doing the same steps here instead means one interpreter (already proven
to run fine via `python script.py`, exactly like `gcc` already does in
this same Makefile) instead of depending on a shell that may not exist.

Run: python run_coverage.py
"""
import glob
import os
import subprocess
import sys

SRC = os.path.join("..", "..", "src")
COVFLAGS = ["-std=c11", "-Wall", "-Wextra", "-O0", "--coverage"]

# (binary name, extra -D flags, source files)
BUILDS = [
    ("cov_hysteresis", [],
     ["test_hysteresis.c", f"{SRC}/hysteresis.c", f"{SRC}/thresholds.c"]),
    ("cov_thresholds", [],
     ["test_thresholds.c", f"{SRC}/thresholds.c"]),
    ("cov_command", [],
     ["test_command.c", f"{SRC}/command.c", f"{SRC}/thresholds.c"]),
    ("cov_fault_handling", [],
     ["test_fault_handling.c", f"{SRC}/fault_handling.c"]),
    ("cov_soil", [],
     ["test_soil.c", f"{SRC}/soil.c"]),
    ("cov_soil_cal_normal",
     ["-DSOIL_ADC_AT_0_PERCENT=100", "-DSOIL_ADC_AT_100_PERCENT=900"],
     ["test_soil_calibrated.c", f"{SRC}/soil.c"]),
    ("cov_soil_cal_inverted",
     ["-DSOIL_ADC_AT_0_PERCENT=900", "-DSOIL_ADC_AT_100_PERCENT=100"],
     ["test_soil_calibrated.c", f"{SRC}/soil.c"]),
    ("cov_dht22_decode", [],
     ["test_dht22_decode.c", f"{SRC}/dht22_decode.c"]),
]


def clean_artifacts() -> None:
    patterns = ["*.gcda", "*.gcno", "*.gcov", "cov_*"]
    for pattern in patterns:
        for path in glob.glob(pattern):
            os.remove(path)


def main() -> None:
    clean_artifacts()

    for name, extra_flags, sources in BUILDS:
        command = ["gcc", *COVFLAGS, *extra_flags, *sources, "-o", name]
        print(" ".join(command))
        result = subprocess.run(command)
        if result.returncode != 0:
            sys.exit(f"Echec de compilation : {name}")

    for name, _, _ in BUILDS:
        # Same invocation style as the Makefile's own "run" target
        # (./name, no extension), which is already proven to work here.
        subprocess.run([f"./{name}"], stdout=subprocess.DEVNULL)

    subprocess.run(["gcovr", "--root", "../..", "--filter", "../../src/",
                     "--txt-metric", "branch", "-s", "."])


if __name__ == "__main__":
    main()
