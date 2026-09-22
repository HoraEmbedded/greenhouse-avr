#include "analog.h"
#include <Arduino.h>
#include "board.h"

void analog_init(void)
{
    analogReadResolution(ADC_RESOLUTION);
    pinMode(PIN_WATER_LEVEL, INPUT_PULLUP);
}

uint16_t analog_read_soil_raw(void)
{
    return (uint16_t)analogRead(PIN_SOIL_ADC);
}

bool water_level_ok(void)
{
    return digitalRead(PIN_WATER_LEVEL) == LOW;
}