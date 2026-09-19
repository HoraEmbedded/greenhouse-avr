#ifndef EEPROM_CONFIG_H
#define EEPROM_CONFIG_H

#include "thresholds.h"

/* Loads config; falls back to defaults (and saves them) if EEPROM is
 * blank, corrupted, or fails validation. */
void eeprom_config_load(GreenhouseThresholds *out);

/* Saves with magic byte + checksum. Uses update_* (skips unchanged
 * bytes -- EEPROM has a ~100k write-cycle limit). */
void eeprom_config_save(const GreenhouseThresholds *cfg);

#endif
