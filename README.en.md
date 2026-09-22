# Regulation system: from ATmega2560 to ESP32 IoT

[![V1 CI](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/ci.yml/badge.svg)](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/ci.yml)
[![V2 CI](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/v2_esp32_ci.yml/badge.svg)](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/v2_esp32_ci.yml)

Monorepo documenting the full evolution of a critical greenhouse
regulation system. From an 8-bit bare-metal firmware validated by 98
unit tests, to a secure 32-bit IoT architecture meeting Industry 4.0
requirements.

## Repository contents

| Folder | Description |
|---|---|
| [`firmware_v1_bare_metal/`](firmware_v1_bare_metal/) | V1: pure C firmware on ATmega2560, drivers written from the datasheet, local hysteresis control |
| [`firmware_v2_rtos_iot/`](firmware_v2_rtos_iot/) | V2: ESP32 + FreeRTOS migration, MQTT/TLS telemetry to Cloud, LCD display, dual simulation/hardware environment |

## Approach

V1 demonstrates mastery of hard real-time constraints: non-blocking
super-loop, IDLE sleep, hardware watchdog, worst-case stack analysis by
real disassembly, algebraic proof of EEPROM lifetime. V2 keeps that
rigor and adds what Industry 4.0 demands: secure connectivity,
preemptive multitasking, separation of concerns, Cloud telemetry.

The decision core (hysteresis, pump guards, sensor degradation) stays
in pure C, portable and host-testable, regardless of the target.

## V1 / V2 comparison

| | V1 Bare-metal | V2 RTOS / IoT |
|---|---|---|
| Target | ATmega2560, 8-bit | ESP32, 32-bit dual-core |
| Architecture | Non-blocking super-loop | 4 preemptive FreeRTOS tasks |
| Communication | Sequential C module calls | Inter-task Queues and Mutex |
| Network | None | WPA2 Wi-Fi, MQTT over TLS 1.2/1.3 |
| Security | Watchdog, checksum, EEPROM | Watchdog, TLS, Root CA, isolated secrets |
| Host tests | 12 suites, 98 cases | 4 suites, 11 cases |
| CI | 6 jobs | 4 jobs |
| Simulation | Wokwi CLI | Wokwi CLI |
| Local output | LCD 1602 I2C | LCD 1602 I2C |
| Remote output | Serial port | Cloud broker (HiveMQ) |

## Quick start

```bash
# V1
cd firmware_v1_bare_metal
pio run

# V2 simulation
cd firmware_v2_rtos_iot
pio run -e esp32dev_wokwi
wokwi-cli .
```

Detailed documentation for each version lives in its own folder.

## Author

Personal project, Horacia Azonhoumon.
