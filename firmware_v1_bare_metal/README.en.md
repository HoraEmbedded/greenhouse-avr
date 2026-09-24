# Automated Greenhouse — ATmega2560

[![CI](https://github.com/<user>/<repo>/actions/workflows/ci.yml/badge.svg)](https://github.com/<user>/<repo>/actions/workflows/ci.yml)

Bare-metal firmware (Arduino Mega / ATmega2560) for automated greenhouse management: temperature/humidity (DHT22), soil moisture (ADC), fan and pump driven by hysteresis, EEPROM-backed configurable thresholds via serial commands, pump safety (water level + daytime-only irrigation), Wokwi simulation.

**Full documentation** (methodology, design decisions, detailed results, step-by-step guide, screenshots): [`docs/GUIDE_COMPLET.en.md`](docs/GUIDE_COMPLET.en.md).

## What this project demonstrates

- Decision logic separated from hardware — testable without a microcontroller (12 suites, 98 cases, native `gcc`)
- Dependability: hardware watchdog, graceful degradation on sensor failure, two independent pump safety interlocks
- Formal worst-case stack analysis (63/8192 bytes, verified by real disassembly)
- Algebraic proof on EEPROM lifetime (not a rough estimate)
- Full CI verification pipeline (6 jobs): build, tests, coverage, static analysis, stack analysis, telemetry
- Python capture/visualization tooling, with fully automated Wokwi simulation (`wokwi-cli`)

## Hardware components

| Component | Role | Interface |
|---|---|---|
| DHT22 | Air temperature and humidity | 1-Wire, pin 2 |
| Potentiometer | Simulates a soil moisture probe | ADC, A0 |
| DS1307 | Real-time clock — daytime-only irrigation | I2C (0x68), shared with the display |
| Float switch | Reservoir water level — pump safety | Digital, pin 3 |
| LCD 1602 | Local display (temperature, humidity, states) | I2C (0x27) |
| Blue / green LED | Simulate the pump / fan relays | Digital, pins 8/9 |

## Software architecture

| Module | Role |
|---|---|
| `hysteresis.c` | Fan/pump decision (dead band) |
| `thresholds.c` | Threshold validation |
| `command.c` | Serial command parser (`GET`/`SET`/`RESET`) |
| `eeprom_config.c` | Persistence (magic byte + XOR checksum) |
| `fault_handling.c` | Degradation after repeated DHT22 failures |
| `ring_buffer.c` | Interrupt-driven UART reception |
| `water_level.c` / `schedule.c` | Pump safety interlocks (water, day/night) |
| `rtc_decode.c` / `dht22_decode.c` / `soil.c` | Sensor decoding |

## Verified numbers

| | |
|---|---|
| Host tests | 12 suites, 98 cases |
| Coverage | 100% lines, 100% functions, 93.5% branches (86/92 — the rest proven unreachable) |
| Worst-case stack | 63 / 8192 bytes (99.2% margin) |
| Flash / RAM | ~4.1 KB / 425 bytes out of 253,952 / 8192 |
| EEPROM lifetime | > 50 years normal use, 1 year even under deliberate abuse |

## Quick start

```bash
# Firmware
pio run

# Unit tests + coverage
cd test/host && make run && make coverage

# Static and stack analysis
cppcheck --enable=warning,style,performance,portability --inconclusive --std=c11 -Isrc src/*.c
python tools/stack_analysis.py

# Telemetry (automated Wokwi capture)
cd tools/telemetry
pip install -r requirements.txt
python generate_drift_scenario.py --output drift_scenario.yaml
python capture_and_plot.py --scenario drift_scenario.yaml --duration 100
```

Full step-by-step procedure and troubleshooting: [`docs/GUIDE_COMPLET.en.md`](docs/GUIDE_COMPLET.en.md).

## Known limitations

- Soil probe simulated by a potentiometer — real calibration needed on an actual capacitive probe
- No validation on real silicon; everything is verified in simulation + host tests
- Day/night window is a compile-time constant, not yet EEPROM-configurable
