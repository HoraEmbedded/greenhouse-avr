#ifndef PLATFORM_LCD_H
#define PLATFORM_LCD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void lcd_init(void);

void lcd_show_ready(void);
void lcd_show_status(float temp_c, float soil_pct, bool fan_on, bool pump_on, bool water_ok);
void lcd_show_sensor_error(uint8_t failures);

#ifdef __cplusplus
}
#endif

#endif