#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "hysteresis.h"
#include "soil.h"
#include "thresholds.h"
#include "command.h"
#include "eeprom_config.h"
#include "fault_handling.h"
#include "dht22_decode.h"
#include "ring_buffer.h"
#include "water_level.h"
#include "rtc_decode.h"
#include "schedule.h"

/* Disable any watchdog left running from a prior reset (avoids a reset loop). */
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) { MCUSR = 0; wdt_disable(); return; }

// ---------------------------------------------------------------------------
// UART
// ---------------------------------------------------------------------------
#define BAUD 9600
#define BRC ((F_CPU/16/BAUD) - 1)

static volatile RingBuffer uart_rx;

void uart_init() { UBRR0H = (BRC>>8); UBRR0L = BRC; UCSR0B = (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0); UCSR0C = (1<<UCSZ01)|(1<<UCSZ00); ring_buffer_init((RingBuffer *)&uart_rx); }
void uart_send_char(char c) { while (!(UCSR0A & (1<<UDRE0))); UDR0 = c; }
void uart_send_string(const char* str) { while (*str) uart_send_char(*str++); }

/* RX buffering: decision logic in ring_buffer.c, tested there. */
ISR(USART0_RX_vect) {
    ring_buffer_push((RingBuffer *)&uart_rx, UDR0);
}

/* Safe cast: ISR only ever writes .head, main() only ever writes .tail. */
uint8_t uart_available(void) { return ring_buffer_available((RingBuffer *)&uart_rx); }
char uart_read_char(void) { return (char)ring_buffer_pop((RingBuffer *)&uart_rx); }

void uart_send_int(int num) {
    char buffer[10]; int i = 0;
    if (num == 0) { uart_send_char('0'); return; }
    if (num < 0) { uart_send_char('-'); num = -num; }
    while (num > 0) { buffer[i++] = (num % 10) + '0'; num /= 10; }
    while (i > 0) { uart_send_char(buffer[--i]); }
}

// ---------------------------------------------------------------------------
// I2C + LCD (4-bit) + RTC (DS1307)
// ---------------------------------------------------------------------------
#define LCD_ADDR 0x27

void i2c_init() { PORTD |= (1<<PD0)|(1<<PD1); TWSR = 0x00; TWBR = 72; TWCR = (1<<TWEN); }
void i2c_start() { TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); while (!(TWCR & (1<<TWINT))); }

/* Must wait for TWSTO to clear (STOP doesn't raise TWINT); otherwise
 * a second I2C device's transaction right after can corrupt the bus. */
void i2c_stop() { TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); while (TWCR & (1<<TWSTO)); }
void i2c_write(uint8_t data) { TWDR = data; TWCR = (1<<TWINT)|(1<<TWEN); while (!(TWCR & (1<<TWINT))); }
uint8_t i2c_read_ack() { TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA); while (!(TWCR & (1<<TWINT))); return TWDR; }
uint8_t i2c_read_nack() { TWCR = (1<<TWINT)|(1<<TWEN); while (!(TWCR & (1<<TWINT))); return TWDR; }

/* DS1307 (natively simulated by Wokwi, unlike DS3231). Reg 0x02 = hours,
 * bit 6 = 12/24h mode (masked; device defaults to 24h). */
#define DS1307_ADDR 0x68
uint8_t rtc_read_hour(void) {
    i2c_start();
    i2c_write(DS1307_ADDR << 1);
    i2c_write(0x02);
    i2c_start();
    i2c_write((DS1307_ADDR << 1) | 1);
    uint8_t raw = i2c_read_nack();
    i2c_stop();
    return bcd_to_decimal(raw & 0x3F);
}

void lcd_send_nibble(uint8_t half_byte, uint8_t mode) {
    uint8_t data = half_byte | mode | 0x08; 
    i2c_start(); i2c_write(LCD_ADDR << 1); i2c_write(data | 0x04); i2c_write(data & ~0x04); i2c_stop(); _delay_us(100);
}

void lcd_send(uint8_t value, uint8_t mode) { lcd_send_nibble(value & 0xF0, mode); lcd_send_nibble((value << 4) & 0xF0, mode); }

void lcd_init() {
    _delay_ms(50); lcd_send_nibble(0x30, 0); _delay_ms(5); lcd_send_nibble(0x30, 0); _delay_us(150);
    lcd_send_nibble(0x30, 0); lcd_send_nibble(0x20, 0); lcd_send(0x28, 0); lcd_send(0x0C, 0); lcd_send(0x01, 0); _delay_ms(2);
}

void lcd_print(const char* str) { while (*str) lcd_send(*str++, 1); }
void lcd_set_cursor(uint8_t col, uint8_t row) { static const uint8_t row_offsets[] = { 0x00, 0x40 }; lcd_send(0x80 | (col + row_offsets[row]), 0); }

void lcd_print_int(int num) {
    char buffer[10]; int i = 0;
    if (num == 0) { lcd_send('0', 1); return; }
    if (num < 0) { lcd_send('-', 1); num = -num; } 
    while (num > 0) { buffer[i++] = (num % 10) + '0'; num /= 10; }
    while (i > 0) { lcd_send(buffer[--i], 1); }
}

// ---------------------------------------------------------------------------
// DHT22 (1-Wire)
// ---------------------------------------------------------------------------
#define DHT_PIN PE4
uint8_t dht_data[5];

/* Interrupts disabled during the bus read: timing is microsecond-critical. */
int dht_read(void) {
    uint8_t i, j;
    volatile uint16_t timeout;
    int result = 0;

    cli();

    DDRE |= (1 << DHT_PIN);
    PORTE &= ~(1 << DHT_PIN);
    _delay_ms(20);
    PORTE |= (1 << DHT_PIN);
    _delay_us(30);
    DDRE &= ~(1 << DHT_PIN);

    timeout = 10000;
    while (PINE & (1 << DHT_PIN)) { if (--timeout == 0) { result = -1; goto done; } }
    timeout = 10000;
    while (!(PINE & (1 << DHT_PIN))) { if (--timeout == 0) { result = -2; goto done; } }
    timeout = 10000;
    while (PINE & (1 << DHT_PIN)) { if (--timeout == 0) { result = -3; goto done; } }

    for (i = 0; i < 5; i++) {
        dht_data[i] = 0;
        for (j = 0; j < 8; j++) {
            timeout = 10000;
            while (!(PINE & (1 << DHT_PIN))) { if (--timeout == 0) { result = -4; goto done; } }
            _delay_us(40);
            if (PINE & (1 << DHT_PIN)) {
                dht_data[i] |= (1 << (7 - j));
                timeout = 10000;
                while (PINE & (1 << DHT_PIN)) { if (--timeout == 0) { result = -5; goto done; } }
            }
        }
    }

    if (!dht22_checksum_valid(dht_data)) {
        result = -6;
    }

done:
    sei();
    return result;
}

// ---------------------------------------------------------------------------
// ADC + water level
// ---------------------------------------------------------------------------
void adc_init() {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read() {
    ADCSRA |= (1 << ADSC);
    while(ADCSRA & (1 << ADSC));
    return ADC;
}

/* Float switch, PE5. Decision logic lives in water_level.c. */
uint8_t water_present(void) { return !(PINE & (1 << PE5)); }

// ---------------------------------------------------------------------------
// Timer1 -- 2s measurement cadence
// ---------------------------------------------------------------------------
volatile uint8_t measure_flag = 0;

void timer1_init() {
    TCCR1A = 0; 
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); 
    OCR1A = 31249; 
    TIMSK1 = (1 << OCIE1A); 
}

ISR(TIMER1_COMPA_vect) {
    measure_flag = 1; 
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    uart_init(); i2c_init(); lcd_init(); adc_init(); timer1_init();

    DDRH |= (1 << PH5) | (1 << PH6);  /* pump, fan */

    /* Float switch input, pull-up. Open/broken wire reads HIGH = "absent". */
    DDRE &= ~(1 << PE5);
    PORTE |= (1 << PE5);

    uint8_t fan_state = 0; 
    uint8_t pump_state = 0;
    uint8_t dht_consecutive_failures = 0;

    GreenhouseThresholds config;
    eeprom_config_load(&config);

    static char cmd_buffer[32];
    uint8_t cmd_len = 0;

    lcd_print("System Active");
    _delay_ms(2000); 
    
    sei(); 

    wdt_enable(WDTO_2S);
    set_sleep_mode(SLEEP_MODE_IDLE);

    while (1) {
        wdt_reset();

        while (uart_available()) {
            char c = uart_read_char();
            if (c == '\r' || c == '\n') {
                if (cmd_len > 0) {
                    cmd_buffer[cmd_len] = '\0';
                    CommandResult result = command_process(cmd_buffer, &config);
                    switch (result) {
                        case CMD_GET:
                            uart_send_string("FAN_ON=");   uart_send_int(config.fan_on_decidegC);
                            uart_send_string(" FAN_OFF="); uart_send_int(config.fan_off_decidegC);
                            uart_send_string(" PUMP_ON="); uart_send_int(config.pump_on_percent);
                            uart_send_string(" PUMP_OFF="); uart_send_int(config.pump_off_percent);
                            uart_send_string("\r\n");
                            break;
                        case CMD_SET_OK:
                            eeprom_config_save(&config);
                            uart_send_string("OK\r\n");
                            break;
                        case CMD_RESET_OK:
                            eeprom_config_save(&config);
                            uart_send_string("RESET_OK\r\n");
                            break;
                        case CMD_ERR_UNKNOWN:
                            uart_send_string("ERR unknown command\r\n");
                            break;
                        case CMD_ERR_BAD_NUMBER:
                            uart_send_string("ERR bad number\r\n");
                            break;
                        case CMD_ERR_REJECTED:
                            uart_send_string("ERR rejected\r\n");
                            break;
                    }
                }
                cmd_len = 0;
            } else if (cmd_len < sizeof(cmd_buffer) - 1) {
                cmd_buffer[cmd_len++] = c;
            }
        }

        if (measure_flag == 1) {
            measure_flag = 0; 

            int dht_state;
            uint16_t raw_soil_value;
            int soil_hum_percent;

            dht_state = dht_read();
            raw_soil_value = adc_read();
            soil_hum_percent = soil_percent_from_raw(raw_soil_value);

            /* Pump doesn't depend on DHT22 -- soil comes from the ADC. */
            pump_state = pump_hysteresis(pump_state, (int8_t)soil_hum_percent, &config);

            uint8_t water_ok = water_present();
            pump_state = pump_output_state(pump_state, water_ok);

            uint8_t current_hour = rtc_read_hour();
            uint8_t daytime = is_daytime(current_hour);
            if (!daytime) pump_state = 0;

            if (pump_state) PORTH |= (1 << PH5); else PORTH &= ~(1 << PH5);
            
            lcd_send(0x01, 0);
            _delay_ms(2);
            
            if (dht_state == 0) {
                dht_consecutive_failures = 0;

                int temperature, air_hum;
                Dht22Reading reading = dht22_decode(dht_data);
                air_hum = reading.air_humidity_decipercent;
                temperature = reading.temperature_decidegC;

                fan_state = fan_hysteresis(fan_state, (int16_t)temperature, &config);
                if (fan_state) PORTH |= (1 << PH6); else PORTH &= ~(1 << PH6);

                lcd_set_cursor(0, 0);
                lcd_print("T:"); lcd_print_int(temperature/10); lcd_print(" F:"); lcd_print(fan_state ? "ON " : "OFF");
                lcd_set_cursor(0, 1);
                lcd_print("S:"); lcd_print_int(soil_hum_percent); lcd_print("% P:"); lcd_print(pump_state ? "ON " : "OFF");
                if (!water_ok) lcd_print(" LOW");

                uart_send_string("T:"); uart_send_int(temperature/10);
                uart_send_string("C | Air:"); uart_send_int(air_hum/10);
                uart_send_string("% | Soil:"); uart_send_int(soil_hum_percent);
                uart_send_string("% | Fan:"); uart_send_string(fan_state ? "ON" : "OFF");
                uart_send_string(" | Pump:"); uart_send_string(pump_state ? "ON" : "OFF");
                uart_send_string(" | Water:"); uart_send_string(water_ok ? "OK" : "LOW");
                uart_send_string(" | Hour:"); uart_send_int(current_hour); uart_send_string("\r\n");
            } else {
                /* Pump unaffected. Fan needs temp, so degrade after repeats. */
                if (dht_consecutive_failures < 255) dht_consecutive_failures++;
                fan_state = degraded_fan_state(dht_consecutive_failures, fan_state);
                if (fan_state) PORTH |= (1 << PH6); else PORTH &= ~(1 << PH6);

                lcd_print("Air Sensor Err");
                if (dht_consecutive_failures >= DHT_FAILURE_SAFETY_THRESHOLD) {
                    lcd_set_cursor(0, 1);
                    lcd_print("Fan forced ON");
                }

                uart_send_string("SENSOR_ERR consecutive=");
                uart_send_int(dht_consecutive_failures);
                uart_send_string(" | Fan:"); uart_send_string(fan_state ? "ON" : "OFF");
                uart_send_string(" | Soil:"); uart_send_int(soil_hum_percent);
                uart_send_string("% | Pump:"); uart_send_string(pump_state ? "ON" : "OFF");
                uart_send_string(" | Water:"); uart_send_string(water_ok ? "OK" : "LOW");
                uart_send_string(" | Hour:"); uart_send_int(current_hour); uart_send_string("\r\n");
            }
        }

        /* Sleep until next interrupt. cli-check-sleep avoids missing a
         * wake-up that arrives between the check and sleep_cpu(). */
        cli();
        if (!uart_available() && measure_flag == 0) {
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        } else {
            sei();
        }
    }
    
    return 0;
}
