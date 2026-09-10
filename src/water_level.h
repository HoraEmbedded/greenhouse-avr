#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <stdint.h>

/* Combines the pump's hysteresis decision with the reservoir's water
 * level. The pump is forced OFF whenever water is not confirmed
 * present, regardless of how dry the soil is -- this overrides
 * hysteresis on purpose. A dry reservoir is a hardware safety concern
 * (a real pump running dry can be damaged), and that takes priority
 * over irrigation timing.
 *
 * Pure function: no I/O, testable on the host
 * (test/host/test_water_level.c). The GPIO read itself (main.c) is
 * wired so that an open circuit -- water absent, OR a disconnected/
 * broken wire -- reads as "not present": a wiring fault fails toward
 * blocking irrigation, not toward running the pump unsupervised. */
uint8_t pump_output_state(uint8_t hysteresis_pump_state, uint8_t water_present);

#endif
