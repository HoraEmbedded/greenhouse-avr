**File: `firmware_v2_rtos_iot/README.en.md`**

```markdown
# Connected greenhouse: ESP32 + FreeRTOS

[![CI](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/v2_esp32_ci.yml/badge.svg)](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/v2_esp32_ci.yml)

IoT evolution of the V1 firmware (ATmega2560 bare-metal) to a 32-bit ESP32
target. Preemptive FreeRTOS multitasking, local hysteresis control kept
identical, secure MQTT/TLS telemetry to a Cloud broker, LCD 1602 I2C
shared on a mutex-protected bus.

## What this project demonstrates

- Bare-metal to RTOS migration: split into 4 preemptive tasks
  (Sensors, Logic, Network, Display), inter-task communication via Queues,
  mutual exclusion on the I2C bus
- Portable business logic: decision modules (hysteresis, pump guards,
  sensor degradation) stay in pure C, testable on PC regardless of target
- Edge-to-Cloud security: WPA2 Wi-Fi, MQTT over TLS 1.2/1.3 (port 8883),
  Root CA certificate authentication, structured JSON payload
- Dual build environment: Wokwi simulation (serial publisher) and real
  board (MQTT/TLS publisher), selected via source filter
- Secret isolation: `secrets.h` git-ignored, `.example` template provided
  for CI

## Hardware components

| Component | Role | Interface |
|---|---|---|
| DHT22 | Air temperature and humidity | GPIO 4 |
| Potentiometer | Simulates a soil moisture probe | ADC, GPIO 34 |
| DS1307 | Real-time clock, daytime-only irrigation | I2C (0x68) |
| Float switch | Reservoir water level, pump safety | GPIO 27 |
| LCD 1602 | Local display (T°, soil, actuator states) | I2C (0x27) |
| Blue / green LED | Simulate pump / fan relays | GPIO 26 / 25 |

## Software architecture

| Task | Priority | Core | Role |
|---|---|---|---|
| `Task_Sensors` | 3 | 0 | Sensor reads at 20 Hz, I2C mutex |
| `Task_Logic` | 2 | 0 | Hysteresis, pump guards, JSON encoder |
| `Task_Network` | 1 | 1 | Telemetry publish at 1 Hz |
| `Task_Display` | 1 | 0 | LCD refresh at 2 Hz |

| `core/` module | Role |
|---|---|
| `hysteresis.c` | Fan/pump decision (dead band) |
| `thresholds.c` | Configuration struct and validation |
| `fault_handling.c` | Fan degradation after repeated DHT22 failures |
| `water_level.c` / `schedule.c` | Pump guards (water, day/night) |
| `rtc_decode.c` / `dht22_decode.c` / `soil.c` | Sensor decoding |
| `telemetry_json.c` | Portable JSON encoder (host-tested) |

## Verified numbers

| | |
|---|---|
| Host tests | 4 suites, 11 cases (native `gcc`) |
| FreeRTOS tasks | 4 (spread over 2 cores) |
| PlatformIO environments | 2 (Wokwi simulation, real board) |
| CI jobs | 4 (host tests, Wokwi build, hardware build, secrets check) |
| Sensor rate | 20 Hz |
| Publish rate | 1 Hz |

## Quick start

### Wokwi simulation

```bash
pio run -e esp32dev_wokwi
wokwi-cli .
```

The LCD shows `System Active` for 2 s, then live values.
Move the DHT22 and potentiometer sliders in Wokwi to test hysteresis
and pump guards.

### Real ESP32 board

```bash
cp include/secrets.h.example include/secrets.h
# Fill in Wi-Fi, MQTT broker, credentials and Root CA
pio run -e esp32dev_hw -t upload
pio device monitor -e esp32dev_hw
```

### Host tests

```bash
cd test/host && make run
```

## Known limitations

- Cloud validation (HiveMQ) requires real silicon, the free Wokwi
  simulation does not bridge Wi-Fi to the Internet
- Soil probe simulated by a potentiometer, real calibration needed on a
  capacitive probe
- `PubSubClient` does not support QoS 1/2, consider ESP-MQTT for
  industrial-grade reliability
- Day/night window is a compile-time constant
```

