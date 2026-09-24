#include <avr/eeprom.h>
#include "eeprom_config.h"

#define EE_MAGIC 0xA5

static uint8_t * const EE_MAGIC_ADDR  = (uint8_t *)0;
/* Not a real pointer -- an EEPROM byte offset (standard avr-libc idiom). */
// cppcheck-suppress intToPointerCast
static void    * const EE_CONFIG_ADDR = (void *)1;

static uint8_t checksum_of(const GreenhouseThresholds *t) {
    const uint8_t *bytes = (const uint8_t *)t;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < sizeof(*t); i++) sum ^= bytes[i];
    return sum;
}

void eeprom_config_load(GreenhouseThresholds *out) {
    uint8_t magic = eeprom_read_byte(EE_MAGIC_ADDR);

    if (magic == EE_MAGIC) {
        GreenhouseThresholds candidate;
        eeprom_read_block(&candidate, EE_CONFIG_ADDR, sizeof(candidate));
        uint8_t stored_checksum =
            eeprom_read_byte((uint8_t *)EE_CONFIG_ADDR + sizeof(candidate));

        if (stored_checksum == checksum_of(&candidate) &&
            thresholds_valid(&candidate)) {
            *out = candidate;
            return;
        }
    }

    GreenhouseThresholds defaults = GREENHOUSE_DEFAULT_THRESHOLDS;
    *out = defaults;
    eeprom_config_save(out);
}

void eeprom_config_save(const GreenhouseThresholds *cfg) {
    eeprom_update_block(cfg, EE_CONFIG_ADDR, sizeof(*cfg));
    eeprom_update_byte((uint8_t *)EE_CONFIG_ADDR + sizeof(*cfg),
                        checksum_of(cfg));
    eeprom_update_byte(EE_MAGIC_ADDR, EE_MAGIC);
}
