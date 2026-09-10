"""Worst-case stack usage analysis for the greenhouse firmware.

Two things this deliberately does NOT do:

  - It does not auto-detect the call graph. Reliably parsing C calls
    needs a real compiler front-end; a homegrown regex scan would be
    exactly the kind of over-trusted approximation this project has
    flagged elsewhere (see the Sentinelle project's NC-015 -- a wrong
    measurement is worse than no measurement). The call graph below was
    read directly from src/main.c and the other src/*.c files, and the
    two deepest chains were cross-checked against the actual disassembly
    (avr-objdump -d) rather than trusted on the compiler's number alone
    -- see the comment on ISR_NAME for what that check found.

  - It does not simulate or estimate frame sizes. It sums the real
    per-function numbers avr-gcc's own -fstack-usage reports (the *.su
    files produced alongside compilation), the same source of truth
    Sentinelle's tools/stack_wcs.py uses for the same reason.

Run (recompiles with -fstack-usage first):
    python stack_analysis.py
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"

SOURCES = ["main.c", "hysteresis.c", "soil.c", "thresholds.c", "command.c",
           "eeprom_config.c", "fault_handling.c", "dht22_decode.c"]

# Call graph, hand-traced from the source (only edges that matter for
# finding the deepest chain -- leaves that nothing here calls further
# don't need an entry). Each edge is commented with what in the source
# justifies it, so it can be checked against src/main.c directly instead
# of taken on faith.
CALL_GRAPH = {
    "main": ["lcd_print_int", "uart_send_int", "command_process",
             "eeprom_config_load", "dht_read", "lcd_set_cursor", "lcd_print"],
    "lcd_print_int": ["lcd_send"],       # lcd_send('0'/'-'/digit, 1)
    "lcd_print": ["lcd_send"],           # while (*str) lcd_send(*str++, 1)
    "lcd_set_cursor": ["lcd_send"],      # lcd_send(0x80 | ..., 0)
    "lcd_send": ["lcd_send_nibble"],     # two sequential calls, same depth
    "lcd_send_nibble": ["i2c_start", "i2c_write", "i2c_stop"],  # tied leaves
    "uart_send_int": ["uart_send_char"],
    "command_process": ["starts_with", "str_equal", "looks_like_number",
                         "parse_int", "thresholds_valid"],
    "eeprom_config_load": ["eeprom_config_save", "thresholds_valid"],
    "eeprom_config_save": [],  # checksum_of() was inlined into both callers
                                # -- confirmed by its absence as its own
                                # entry in the .su output, not assumed
}

# The two interrupt sources in this firmware. On AVR, without an ISR
# explicitly re-enabling interrupts (sei()) inside itself -- neither does
# here -- at most ONE ISR is ever active at a time: they can preempt
# main-line code, but never each other. So the worst case adds whichever
# SINGLE ISR costs more, not both summed.
#
# Both numbers below were cross-checked by disassembly (avr-objdump -d),
# not taken from -fstack-usage alone:
#   __vector_17 (Timer1, measurement cadence): 3-byte HW return address +
#     4 explicit pushes (r1, r0, SREG, r24) = 7.
#   __vector_25 (USART0 RX): 3-byte HW return address + N explicit
#     pushes, where N depends on whether GCC inlines ring_buffer_push()
#     into the ISR or emits a real call to it -- and that decision is
#     NOT fixed by this file alone. It shifted from 9 pushes (12 bytes
#     total) to 16 pushes (19 bytes total) after an unrelated one-line
#     change elsewhere in main.c (making lcd_set_cursor's local array
#     `static const`), with nothing touched in the ISR or ring_buffer.c.
#     GCC's inlining cost model at -Os weighs the whole translation
#     unit, so shrinking one function can tip the scale for inlining
#     decisions in a completely different one. This is exactly why this
#     script recompiles from source every run instead of caching a
#     number: the worst case is a property of the compiled BINARY, not
#     of the source code read in isolation, and it can move without any
#     change to the function being measured.
ISR_CANDIDATES = ["__vector_17", "__vector_25"]

SU_LINE = re.compile(r"^[^:]+:\d+:\d+:(\w+)\t(\d+)\t(\w+)")


def compile_and_collect() -> dict:
    """Rebuilds each source with -fstack-usage and parses the resulting
    .su files into {function_name: frame_bytes}."""
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
                print(f"ATTENTION : {name} a une utilisation de pile NON "
                      f"BORNEE ({kind}) -- cette analyse ne la couvre pas",
                      file=sys.stderr)
            sizes[name] = int(frame_bytes)
    return sizes


def deepest_chain(sizes: dict, node: str) -> tuple:
    """Returns (total_bytes, path) for the heaviest root-to-leaf path
    starting at `node`. Ties are broken by list order in CALL_GRAPH,
    which doesn't matter here since tied branches (see lcd_send_nibble)
    are the same size regardless of which is picked."""
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
        print(f"ATTENTION : {missing} absent(s) de la sortie .su -- "
              f"le graphe d'appel ci-dessus est peut-etre perime",
              file=sys.stderr)

    total, path = deepest_chain(sizes, "main")
    isr_sizes = {name: sizes.get(name, 0) for name in ISR_CANDIDATES}
    worst_isr_name = max(isr_sizes, key=isr_sizes.get)
    isr_bytes = isr_sizes[worst_isr_name]
    worst_case = total + isr_bytes

    print("Chaine d'appel la plus profonde (code principal, hors interruption) :")
    running = 0
    for name in path:
        running += sizes.get(name, 0)
        print(f"  {name:<20} {sizes.get(name, 0):>3} octets  "
              f"(cumul: {running} octets)")

    print(f"\nSous-total main-line               : {total} octets")
    print("Cout de chaque source d'interruption (une seule active a la fois) :")
    for name, size in isr_sizes.items():
        marker = "  <- retenu (le plus couteux)" if name == worst_isr_name else ""
        print(f"  {name:<20} {size:>3} octets{marker}")
    print(f"+ interruption retenue ({worst_isr_name}) : {isr_bytes} octets")
    print(f"= PIRE CAS TOTAL                   : {worst_case} octets")

    sram_total = 8192  # ATmega2560, datasheet section 8.4
    margin = sram_total - worst_case
    print(f"\nSRAM disponible sur l'ATmega2560 : {sram_total} octets")
    print(f"Marge restante                   : {margin} octets "
          f"({100 * margin / sram_total:.1f} % libres)")

    budget = 2048  # generous alert threshold: a quarter of total SRAM.
    # Not "the point at which the firmware breaks" (that's 8192, and this
    # analysis already accounts for the ISR) -- it's an early-warning
    # tripwire so a future change that quietly triples stack depth gets
    # caught in CI, long before it's anywhere near the real ceiling.
    if worst_case > budget:
        print(f"\nDEPASSEMENT DU BUDGET D'ALERTE ({budget} octets) -- "
              f"pas forcement un bug, mais a examiner avant de merger.",
              file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
