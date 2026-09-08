#ifndef FAULT_HANDLING_H
#define FAULT_HANDLING_H

#include <stdint.h>

/* After this many CONSECUTIVE failed DHT22 reads (each read cycle is 2 s,
 * see timer1_init() in main.c), the firmware can no longer trust that
 * last_known_fan_state still reflects reality -- ~10 s of missing data
 * is long enough that the greenhouse's real temperature could have
 * moved well past either threshold. */
#define DHT_FAILURE_SAFETY_THRESHOLD 5

/* Decides what the fan should do once persistent sensor failures make the
 * hysteresis logic in hysteresis.c unusable (there is no temperature to
 * compare against a threshold). Pure function: no I/O, testable on the
 * host (test/host/test_fault_handling.c).
 *
 * Safety choice: force the fan ON, not off, once the threshold is
 * crossed. A greenhouse that overheats can damage or kill its plants
 * within an hour; a fan running for a while it did not strictly need to
 * costs a bit of power and nothing else. Between the two failure modes,
 * this project is biased towards the reversible one. A short outage (below
 * the threshold) is not treated as a reason to change anything yet: it
 * keeps the last known state, on the assumption that a glitch this brief
 * is more likely a missed reading than a real change in the room. */
uint8_t degraded_fan_state(uint8_t consecutive_failures,
                            uint8_t last_known_fan_state);

#endif
