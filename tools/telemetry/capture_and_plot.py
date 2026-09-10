"""One command: runs the firmware in Wokwi headlessly (via wokwi-cli),
captures its real serial output straight to a file, and turns it into a
graph -- no Serial Monitor, no copy-paste.

Setup (once):
    Install wokwi-cli: https://docs.wokwi.com/wokwi-ci/cli-installation
    Get a free API token: https://wokwi.com/dashboard/ci
    setx WOKWI_CLI_TOKEN wok_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx   (Windows)
    # then open a NEW terminal (or fully restart VS Code) so the
    # variable actually takes effect

Usage:
    python capture_and_plot.py --duration 120
    python capture_and_plot.py --duration 300 --output demo_soir.png

    # Re-plot an existing capture without re-running the simulation:
    python capture_and_plot.py --skip-capture --serial-log captured_serial.txt

This reuses the exact same tested building blocks as the rest of
tools/telemetry/: FileSource and retimestamp() from logger_from_file.py,
run() from logger.py, and load_csv()/build_figure() from plot.py. The
only new, untested-by-me code here is the subprocess call to wokwi-cli
itself -- I have no Wokwi account or firmware to actually run in the
environment I'm writing this in, so that one piece is exactly the
command from Wokwi's own CLI documentation, not verified end-to-end on
my side.
"""
import argparse
import subprocess
import sys
from pathlib import Path

from logger import run
from logger_from_file import FileSource, retimestamp
from plot import build_figure, load_csv


def capture(project_dir: str, serial_log: str, duration_s: float) -> None:
    """Runs wokwi-cli for duration_s seconds of simulated time, writing
    the serial output straight to serial_log. Raises SystemExit with a
    clear message if wokwi-cli isn't installed or the run fails --
    deliberately not a bare traceback, since the likely causes (CLI not
    installed, token not set) are both things a person can fix in
    seconds once told which one it is.
    """
    timeout_ms = int(duration_s * 1000)
    print(f"Lancement de la simulation Wokwi pour {duration_s:.0f} s "
          f"(cela peut prendre un peu plus longtemps en temps réel)...")

    # Absolute path only: wokwi-cli resolves relative paths against
    # project_dir (e.g. "../.."), not against the current working
    # directory this script was launched from -- passing a relative
    # --serial-log-file silently looks in the wrong place instead of
    # failing loudly about it.
    serial_log = str(Path(serial_log).resolve())

    try:
        result = subprocess.run(
            ["wokwi-cli", project_dir,
             "--serial-log-file", serial_log,
             "--timeout", str(timeout_ms)],
            capture_output=True, text=True,
        )
    except FileNotFoundError:
        raise SystemExit(
            "wokwi-cli est introuvable. Installe-le avec :\n"
            "    iwr https://wokwi.com/ci/install.ps1 -useb | iex\n"
            "(PowerShell) et vérifie que WOKWI_CLI_TOKEN est bien défini "
            "(https://wokwi.com/dashboard/ci)."
        )

    print(result.stdout)

    # Per Wokwi's docs, exit code 42 (default --timeout-exit-code) means
    # the simulation ran to completion and stopped at the timeout -- the
    # NORMAL, expected outcome here, not a failure.
    if result.returncode not in (0, 42):
        print(result.stderr, file=sys.stderr)
        raise SystemExit(
            f"wokwi-cli a échoué (code {result.returncode}). Vérifie le "
            "message ci-dessus -- une cause fréquente est WOKWI_CLI_TOKEN "
            "absent ou invalide."
        )

    print(f"Capture terminée -> {serial_log}")


def process(serial_log: str, csv_path: str, interval_s: float) -> None:
    """Turns a captured serial log into a timestamped CSV. Identical to
    logger_from_file.py's own main(), factored out so this script can
    call it directly instead of shelling out to a second Python process.
    """
    with open(serial_log, encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    source = FileSource(lines)
    with open(csv_path, "w", newline="", encoding="utf-8") as csv_file:
        try:
            run(source, csv_file)
        except StopIteration:
            pass

    retimestamp(csv_path, interval_s)
    print(f"CSV écrit -> {csv_path}")


def plot(csv_path: str, output_path: str) -> None:
    timestamps, temps, soils, fan, pump, sensor_error = load_csv(csv_path)
    if not timestamps:
        raise SystemExit(
            f"Aucune ligne de télémétrie reconnue dans {csv_path}. "
            "Le firmware a-t-il eu le temps d'envoyer au moins une "
            "lecture pendant la durée capturée ?"
        )
    fig = build_figure(timestamps, temps, soils, fan, pump, sensor_error)
    fig.savefig(output_path, dpi=150)
    print(f"Graphique écrit -> {output_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-dir", default="../..",
                         help="Racine du projet PlatformIO (où se trouvent "
                              "wokwi.toml et diagram.json)")
    parser.add_argument("--duration", type=float, default=120,
                         help="Secondes de temps SIMULÉ à capturer")
    parser.add_argument("--interval", type=float, default=2.0,
                         help="Cadence réelle d'échantillonnage du firmware "
                              "(Timer1 dans main.c), pour l'axe des temps")
    parser.add_argument("--serial-log", default="captured_serial.txt")
    parser.add_argument("--csv", default="telemetry.csv")
    parser.add_argument("--output", default="telemetry.png")
    parser.add_argument("--skip-capture", action="store_true",
                         help="Ne relance pas Wokwi, réutilise "
                              "--serial-log tel quel (pratique pour "
                              "retracer sans rejouer la simulation)")
    args = parser.parse_args()

    if not args.skip_capture:
        capture(args.project_dir, args.serial_log, args.duration)

    process(args.serial_log, args.csv, args.interval)
    plot(args.csv, args.output)


if __name__ == "__main__":
    main()
