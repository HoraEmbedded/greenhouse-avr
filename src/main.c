
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

// Hardware Watchdog Timer disable (executes before main)
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) { MCUSR = 0; wdt_disable(); return; }

// ==============================================================================
// UART DRIVER (Serial Communication)
// ==============================================================================
#define BAUD 9600
#define BRC ((F_CPU/16/BAUD) - 1)

static volatile RingBuffer uart_rx;

void uart_init() { UBRR0H = (BRC>>8); UBRR0L = BRC; UCSR0B = (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0); UCSR0C = (1<<UCSZ01)|(1<<UCSZ00); ring_buffer_init((RingBuffer *)&uart_rx); }
void uart_send_char(char c) { while (!(UCSR0A & (1<<UDRE0))); UDR0 = c; }
void uart_send_string(const char* str) { while (*str) uart_send_char(*str++); }

/* Interrupt-driven reception, replacing the polling that read UDR0
 * directly from main()'s loop. The difference isn't cosmetic: dht_read()
 * blocks for up to ~260 us worst case with interrupts disabled (see its
 * own comment on cli()/sei()), and command_process() itself takes a few
 * microseconds per call -- polling UCSR0A/UDR0 straight from main() means
 * a byte arriving during any of that window, or simply while main() is
 * elsewhere in its loop, could be silently overwritten by the next byte
 * before anyone reads it. The ISR captures every byte the instant it
 * lands, independent of what main() happens to be doing.
 *
 * The buffer itself (push/pop/wraparound/overflow) lives in
 * ring_buffer.c and is tested there on the host -- this ISR is now just
 * the AVR-specific glue around it, with no logic left of its own to get
 * wrong. */
ISR(USART0_RX_vect) {
    ring_buffer_push((RingBuffer *)&uart_rx, UDR0);
}

/* uart_rx is declared volatile because the ISR and main() both touch it
 * asynchronously; ring_buffer_push/pop/available take a plain (non-
 * volatile) pointer, so calling them here casts that qualifier away for
 * the duration of each call. That's sound specifically because of who
 * touches what: ring_buffer_push only ever runs inside the ISR (which
 * cannot itself be interrupted, since nothing here re-enables interrupts
 * inside an ISR), and only ever writes .head; ring_buffer_pop/available
 * only ever run in main() and only ever touch .tail. Neither side can
 * observe the other mid-update of the field IT doesn't own, so there is
 * nothing for the missing volatile to protect against within a single
 * call -- but this is a property of this specific access pattern, not
 * something the cast itself guarantees in general. */
uint8_t uart_available(void) { return ring_buffer_available((RingBuffer *)&uart_rx); }
char uart_read_char(void) { return (char)ring_buffer_pop((RingBuffer *)&uart_rx); }

void uart_send_int(int num) {
    char buffer[10]; int i = 0;
    if (num == 0) { uart_send_char('0'); return; }
    if (num < 0) { uart_send_char('-'); num = -num; }
    while (num > 0) { buffer[i++] = (num % 10) + '0'; num /= 10; }
    while (i > 0) { uart_send_char(buffer[--i]); }
}

// ==============================================================================
// I2C & LCD DRIVER (4-bit Mode)
// ==============================================================================
#define LCD_ADDR 0x27

void i2c_init() { PORTD |= (1<<PD0)|(1<<PD1); TWSR = 0x00; TWBR = 72; TWCR = (1<<TWEN); }
void i2c_start() { TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); while (!(TWCR & (1<<TWINT))); }
void i2c_stop() { TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); }
void i2c_write(uint8_t data) { TWDR = data; TWCR = (1<<TWINT)|(1<<TWEN); while (!(TWCR & (1<<TWINT))); }

/* Reception was never needed until the RTC: the LCD driver only ever
 * writes. TWEA (ack) tells the DS1307 more bytes will be read; its
 * absence (i2c_read_nack) tells it this is the last one -- standard
 * I2C multi-byte read protocol. */
uint8_t i2c_read_ack() { TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA); while (!(TWCR & (1<<TWINT))); return TWDR; }
uint8_t i2c_read_nack() { TWCR = (1<<TWINT)|(1<<TWEN); while (!(TWCR & (1<<TWINT))); return TWDR; }

// DS1307 real-time clock, natively simulated by Wokwi (unlike the
// DS3231, which Wokwi only supports through unofficial community
// custom chips -- see docs.wokwi.com/parts/wokwi-ds1307). Register
// 0x02 is hours; bit 6 selects 12/24-hour mode, masked off here since
// the DS1307 defaults to 24-hour mode on power-up and nothing in this
// firmware ever changes that.
#define DS1307_ADDR 0x68
uint8_t rtc_read_hour(void) {
    i2c_start();
    i2c_write(DS1307_ADDR << 1);
    i2c_write(0x02);
    i2c_start();  // repeated start: switch from write to read
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

// ==============================================================================
// DHT22 SENSOR DRIVER (1-Wire Protocol)
// ==============================================================================
#define DHT_PIN PE4
uint8_t dht_data[5];

/* The DHT22 protocol relies on microsecond-scale pulse widths (40-80 us).
 * Timer1's interrupt (ISR below) only sets a flag and returns quickly, but
 * even that short an interruption lands inside a single bit window here.
 * Interrupts are disabled for the duration of the bus read and restored
 * right after, so a bit can never be mistimed because of the 2 s heartbeat
 * firing at the wrong instant. The read itself takes a few milliseconds,
 * so this has no visible effect on the rest of the system. */
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

// ==============================================================================
// ADC DRIVER (Analog-to-Digital Converter)
// ==============================================================================
void adc_init() {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read() {
    ADCSRA |= (1 << ADSC);
    while(ADCSRA & (1 << ADSC));
    return ADC;
}

// water_present() is the only piece of GPIO glue for the float switch --
// the actual decision (should the pump run given this reading) lives in
// water_level.c and is tested there, same split as everywhere else in
// this firmware.
uint8_t water_present(void) { return !(PINE & (1 << PE5)); }

// ==============================================================================
// INTERRUPT HANDLING (Timer 1)
// ==============================================================================
volatile uint8_t measure_flag = 0;

// Configures Timer1 in CTC mode for a 2s interrupt
void timer1_init() {
    TCCR1A = 0; 
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); 
    OCR1A = 31249; 
    TIMSK1 = (1 << OCIE1A); 
}

ISR(TIMER1_COMPA_vect) {
    measure_flag = 1; 
}

// ==============================================================================
// MAIN PROGRAM
// ==============================================================================
int main(void) {
    uart_init(); i2c_init(); lcd_init(); adc_init(); timer1_init();
    
    // Configure pump (PH5) and fan (PH6) pins as output
    DDRH |= (1 << PH5) | (1 << PH6); 

    // Water level float switch: digital pin 3 (PE5), input with internal
    // pull-up. Wired (see diagram.json) so the switch closes to GND when
    // the float rises -- water present reads LOW. Left open (float down,
    // OR a broken/disconnected wire) reads HIGH via the pull-up: a
    // wiring fault fails toward "no water confirmed", which blocks the
    // pump rather than leaving it free to run unsupervised.
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

    // Hardware safety net: wdt_init() above disabled the watchdog left
    // running from any prior reset (required so a genuine watchdog reset
    // doesn't loop forever resetting itself). Now that initialisation is
    // done, re-enable it deliberately. 2 s comfortably covers the worst
    // case of dht_read()'s own timeout cascade (~260 ms, measured by
    // counting its loop iterations) with margin to spare -- if the
    // firmware ever hangs somewhere that ISN'T a bounded wait like that
    // one, this is what brings it back without someone unplugging it.
    wdt_enable(WDTO_2S);

    set_sleep_mode(SLEEP_MODE_IDLE);

    while (1) {

        // Fed once per loop iteration. The loop's own body (serial
        // command handling, then at most one measurement cycle) is
        // always well under 2 s -- if two consecutive iterations ever
        // exceed that, something is genuinely stuck, and a reset is the
        // correct response, not a longer timeout.
        wdt_reset();

        // Serial command handling, independent of the 2 s measurement
        // cycle: a command typed at any moment is picked up on the next
        // loop iteration, not just right after a measurement.
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
                            uart_send_string("ERR rejected: would make thresholds invalid\r\n");
                            break;
                    }
                }
                cmd_len = 0;
            } else if (cmd_len < sizeof(cmd_buffer) - 1) {
                cmd_buffer[cmd_len++] = c;
            }
            // else: line too long, silently dropped until the next newline
        }

        if (measure_flag == 1) {
            measure_flag = 0; 

            int temperature, air_hum, dht_state;
            uint16_t raw_soil_value;
            int soil_hum_percent;

            dht_state = dht_read();
            raw_soil_value = adc_read();
            soil_hum_percent = soil_percent_from_raw(raw_soil_value);

            // Pump control never depended on the DHT22 in the first
            // place: soil moisture comes from the ADC/potentiometer
            // channel, which has nothing to do with air temperature or
            // humidity. It used to be evaluated only when the DHT22 read
            // succeeded, which meant one unrelated sensor's failure could
            // silently freeze irrigation decisions too. Fixed here: the
            // pump is now driven every cycle, regardless of dht_state.
            pump_state = pump_hysteresis(pump_state, (int8_t)soil_hum_percent, &config);

            // Reservoir safety check overrides hysteresis, not the
            // other way around -- see water_level.c for why. This has
            // to happen AFTER pump_hysteresis() computes its opinion,
            // and BEFORE that opinion reaches the relay.
            uint8_t water_ok = water_present();
            pump_state = pump_output_state(pump_state, water_ok);

            // Daytime-only irrigation: overrides hysteresis the same
            // way the water check does, for an agronomic reason this
            // time rather than a hardware-safety one -- see schedule.h.
            uint8_t current_hour = rtc_read_hour();
            uint8_t daytime = is_daytime(current_hour);
            if (!daytime) pump_state = 0;

            if (pump_state) PORTH |= (1 << PH5); else PORTH &= ~(1 << PH5);
            
            lcd_send(0x01, 0); // Clear LCD
            _delay_ms(2);
            
            if (dht_state == 0) {
                dht_consecutive_failures = 0;

                Dht22Reading reading = dht22_decode(dht_data);
                air_hum = reading.air_humidity_decipercent;
                temperature = reading.temperature_decidegC;
                
                // Decision logic lives in hysteresis.c, unit-tested on the
                // host (test/host/test_hysteresis.c) independently of this
                // firmware -- see that file for why. Thresholds come from
                // `config`, loaded from EEPROM at boot and changeable at
                // runtime via serial commands (see command.c).
                fan_state = fan_hysteresis(fan_state, (int16_t)temperature, &config);
                if (fan_state) PORTH |= (1 << PH6); else PORTH &= ~(1 << PH6);
                
                // Update LCD
                lcd_set_cursor(0, 0);
                lcd_print("T:"); lcd_print_int(temperature/10); lcd_print(" F:"); lcd_print(fan_state ? "ON " : "OFF");
                lcd_set_cursor(0, 1);
                lcd_print("S:"); lcd_print_int(soil_hum_percent); lcd_print("% P:"); lcd_print(pump_state ? "ON " : "OFF");
                if (!water_ok) lcd_print(" LOW");
                
                // Send data via UART
                uart_send_string("T:"); uart_send_int(temperature/10);
                uart_send_string("C | Air:"); uart_send_int(air_hum/10);
                uart_send_string("% | Soil:"); uart_send_int(soil_hum_percent);
                uart_send_string("% | Fan:"); uart_send_string(fan_state ? "ON" : "OFF");
                uart_send_string(" | Pump:"); uart_send_string(pump_state ? "ON" : "OFF");
                uart_send_string(" | Water:"); uart_send_string(water_ok ? "OK" : "LOW");
                uart_send_string(" | Hour:"); uart_send_int(current_hour); uart_send_string("\r\n");
            } else {
                // The pump above is unaffected by this branch (see the
                // comment further up). The fan is a different story: it
                // depends entirely on a temperature this cycle does not
                // have. A single missed reading is not treated as a
                // reason to change anything (probably a timing glitch on
                // the 1-Wire bus), but repeated consecutive failures are
                // -- see fault_handling.c for the reasoning.
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

        // Nothing left to do this iteration: sleep until the next
        // interrupt (Timer1's 2 s tick, or a byte arriving on UART)
        // instead of spinning in a busy-wait for up to 2 seconds every
        // cycle. SLEEP_MODE_IDLE only stops the CPU core -- Timer1, the
        // USART, TWI, and the watchdog all keep running normally, so
        // every wake source this firmware actually needs still works.
        // A deeper mode (Power-down) would also stop the USART's clock,
        // silently breaking command reception.
        //
        // The disable-check-sleep sequence below is not just tidy
        // ordering: it closes a real race. Without it, an interrupt
        // could fire in the gap between "check if there's work" and
        // "go to sleep", and that wake-up would be missed until the
        // NEXT interrupt (up to 2 s later, for a command that should
        // have been answered immediately). AVR guarantees the
        // instruction right after sei() always executes before any
        // pending interrupt is serviced, so sei() immediately followed
        // by sleep_cpu() cannot lose a wake-up that arrives in between.
        cli();
        if (!uart_available() && measure_flag == 0) {
            sleep_enable();
            sei();
            sleep_cpu();   // execution resumes here after any interrupt
            sleep_disable();
        } else {
            sei();
        }
    }
    
    return 0;
}