#ifndef FAULT_HANDLING_H
#define FAULT_HANDLING_H

#include <stdint.h>
#include <stdbool.h> 

#define DHT_FAILURE_SAFETY_THRESHOLD 5

#ifdef __cplusplus
extern "C" {
#endif

/* After N consecutive DHT22 failures, force fan ON (safer failure
 * mode than leaving it in whatever state it was). */
uint8_t degraded_fan_state(uint8_t consecutive_failures,
                            uint8_t last_known_fan_state);

#ifdef __cplusplus
}
#endif

#endif
