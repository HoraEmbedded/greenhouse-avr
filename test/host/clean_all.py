"""Cross-platform replacement for the `clean` recipe -- see
run_coverage.py's docstring for why this isn't `rm -f` directly in the
Makefile.

Run: python clean_all.py
"""
import glob
import os

TEST_BINARIES = [
    "test_hysteresis", "test_thresholds", "test_command",
    "test_fault_handling", "test_soil", "test_soil_calibrated_normal",
    "test_soil_calibrated_inverted", "test_dht22_decode",
]


def main() -> None:
    patterns = ["*.gcda", "*.gcno", "*.gcov", "cov_*"]
    for base in TEST_BINARIES:
        patterns.append(base)
        patterns.append(base + ".exe")

    removed = 0
    for pattern in patterns:
        for path in glob.glob(pattern):
            os.remove(path)
            removed += 1

    print(f"Nettoyé : {removed} fichier(s) supprimé(s).")


if __name__ == "__main__":
    main()
