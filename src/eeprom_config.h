#ifndef EEPROM_CONFIG_H
#define EEPROM_CONFIG_H

#include "thresholds.h"

/* Loads thresholds from EEPROM into *out. If the EEPROM has never been
 * written (fresh chip), or its content fails validation (wrong magic
 * byte, checksum mismatch, or the same range checks used everywhere
 * else), *out is set to the factory defaults and those defaults are
 * written back -- so the state we just decided to trust is the state
 * actually on the chip from this point on, and the NEXT boot finds a
 * valid image instead of repeating the same fallback. */
void eeprom_config_load(GreenhouseThresholds *out);

/* Persists cfg to EEPROM, with a magic byte and checksum so a future load
 * can tell a genuine configuration from an uninitialised or corrupted
 * one. Uses eeprom_update_*, not eeprom_write_*: it skips the write
 * entirely when the byte is already correct, which matters because
 * EEPROM cells wear out after roughly 100 000 write cycles. */
void eeprom_config_save(const GreenhouseThresholds *cfg);

#endif
