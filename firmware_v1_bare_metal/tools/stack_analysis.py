"""Worst-case stack usage analysis.

Call graph is traced by hand from source (not auto-parsed -- regex-based
call graphs are unreliable). Frame sizes come from avr-gcc -fstack-usage.

Run: python stack_analysis.py
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"

SOURCES = ["main.c", "hysteresis.c", "soil.c", "thresholds.c", "command.c",
           "eeprom_config.c", "fault_handling.c", "dht22_decode.c",
           "ring_buffer.c", "water_level.c", "rtc_decode.c", "schedule.c"]

# Only edges needed to find the deepest chain.
CALL_GRAPH = {
    "main": ["lcd_print_int", "uart_send_int", "command_process",
             "eeprom_config_load", "dht_read", "lcd_set_cursor", "lcd_print"],
    "lcd_print_int": ["lcd_send"],
    "lcd_print": ["lcd_send"],
    "lcd_set_cursor": ["lcd_send"],
    "lcd_send": ["lcd_send_nibble"],
    "lcd_send_nibble": ["i2c_start", "i2c_write", "i2c_stop"],
    "uart_send_int": ["uart_send_char"],
    "command_process": ["starts_with", "str_equal", "looks_like_number",
                         "parse_int", "thresholds_valid"],
    "eeprom_config_load": ["eeprom_config_save", "thresholds_valid"],
    "eeprom_config_save": [],
}

# At most one ISR active at a time (no nested interrupts). Take the max.
# Cost depends on GCC inlining decisions for the whole file, not just
# this function -- recompiled fresh every run, not cached.
ISR_CANDIDATES = ["__vector_17", "__vector_25"]

SU_LINE = re.compile(r"^[^:]+:\d+:\d+:(\w+)\t(\d+)\t(\w+)")


def compile_and_collect() -> dict:
    args = ["avr-gcc", "-mmcu=atmega2560", "-DF_CPU=16000000L", "-Os",
            "-std=c11", f"-I{SRC}", "-fstack-usage", "-c"]
    args += [str(SRC / s) for s in SOURCES]
    result = subprocess.run(args, cwd=ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"Compilation failed:\n{result.stderr}")

    sizes = {}
    for su_file in ROOT.glob("*.su"):
        for line in su_file.read_text().splitlines():
            match = SU_LINE.match(line)
            if not match:
                continue
            name, frame_bytes, kind = match.groups()
            if kind != "static":
                print(f"WARNING: {name} has UNBOUNDED ({kind}) stack usage",
                      file=sys.stderr)
            sizes[name] = int(frame_bytes)
    return sizes


def deepest_chain(sizes: dict, node: str) -> tuple:
    own = sizes.get(node, 0)
    children = CALL_GRAPH.get(node, [])
    if not children:
        return own, [node]

    best_total, best_path = 0, []
    for child in children:
        child_total, child_path = deepest_chain(sizes, child)
        if child_total > best_total:
            best_total, best_path = child_total, child_path

    return own + best_total, [node] + best_path


def main() -> None:
    sizes = compile_and_collect()

    missing = [n for n in CALL_GRAPH if n not in sizes]
    if missing:
        print(f"WARNING: {missing} not in .su output -- graph may be stale",
              file=sys.stderr)

    total, path = deepest_chain(sizes, "main")
    isr_sizes = {name: sizes.get(name, 0) for name in ISR_CANDIDATES}
    worst_isr_name = max(isr_sizes, key=isr_sizes.get)
    isr_bytes = isr_sizes[worst_isr_name]
    worst_case = total + isr_bytes

    print("Deepest call chain (main-line, no interrupt):")
    running = 0
    for name in path:
        running += sizes.get(name, 0)
        print(f"  {name:<20} {sizes.get(name, 0):>3} bytes  (running: {running})")

    print(f"\nMain-line subtotal              : {total} bytes")
    print("Cost of each ISR (only one active at a time):")
    for name, size in isr_sizes.items():
        marker = "  <- chosen (worst)" if name == worst_isr_name else ""
        print(f"  {name:<20} {size:>3} bytes{marker}")
    print(f"+ chosen ISR ({worst_isr_name})    : {isr_bytes} bytes")
    print(f"= WORST CASE TOTAL              : {worst_case} bytes")

    sram_total = 8192
    margin = sram_total - worst_case
    print(f"\nSRAM available (ATmega2560): {sram_total} bytes")
    print(f"Margin: {margin} bytes ({100 * margin / sram_total:.1f}% free)")

    budget = 2048
    if worst_case > budget:
        print(f"\nALERT BUDGET EXCEEDED ({budget} bytes) -- review before merging.",
              file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
