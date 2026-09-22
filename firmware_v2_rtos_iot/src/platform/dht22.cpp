#include "dht22.h"
#include <DHTesp.h>
#include "board.h"

static DHTesp s_dht;

void dht22_init(void)
{
    s_dht.setup(PIN_DHT22, DHTesp::DHT22);
}

bool dht22_read(float *temperature_c, float *humidity_pct)
{
    TempAndHumidity r = s_dht.getTempAndHumidity();
    if (isnan(r.temperature) || isnan(r.humidity)) return false;
    *temperature_c = r.temperature;
    *humidity_pct   = r.humidity;
    return true;
}

