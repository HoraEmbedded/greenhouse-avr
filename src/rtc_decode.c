#include "rtc_decode.h"

uint8_t bcd_to_decimal(uint8_t bcd) {
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}
