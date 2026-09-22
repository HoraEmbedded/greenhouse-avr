#include "lcd.h"
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "i2c_bus.h"

#define LCD_ADDR         0x27
#define LCD_COLS         16
#define LCD_ROWS         2
#define I2C_LOCK_TIMEOUT pdMS_TO_TICKS(100)

static LiquidCrystal_I2C s_lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

void lcd_init(void)
{
    s_lcd.init();
    s_lcd.backlight();
}

void lcd_show_ready(void)
{
    if (!i2c_bus_lock(I2C_LOCK_TIMEOUT)) return;
    s_lcd.clear();
    s_lcd.setCursor(0, 0);
    s_lcd.print("System Active");
    i2c_bus_unlock();
}

void lcd_show_status(float temp_c, float soil_pct, bool fan_on, bool pump_on, bool water_ok)
{
    if (!i2c_bus_lock(I2C_LOCK_TIMEOUT)) return;

    char line0[17];
    char line1[17];
    snprintf(line0, sizeof(line0), "T:%.0fC F:%s",
             temp_c, fan_on ? "ON " : "OFF");
    snprintf(line1, sizeof(line1), "S:%.0f%% P:%s%s",
             soil_pct, pump_on ? "ON " : "OFF",
             water_ok ? "" : " LOW");

    s_lcd.setCursor(0, 0);
    s_lcd.print("                ");
    s_lcd.setCursor(0, 0);
    s_lcd.print(line0);

    s_lcd.setCursor(0, 1);
    s_lcd.print("                ");
    s_lcd.setCursor(0, 1);
    s_lcd.print(line1);

    i2c_bus_unlock();
}

void lcd_show_sensor_error(uint8_t failures)
{
    if (!i2c_bus_lock(I2C_LOCK_TIMEOUT)) return;
    s_lcd.clear();
    s_lcd.setCursor(0, 0);
    s_lcd.print("Air Sensor Err");
    s_lcd.setCursor(0, 1);
    if (failures >= 3) {
        s_lcd.print("Fan forced ON");
    } else {
        s_lcd.print("Recovering...");
    }
    i2c_bus_unlock();
}