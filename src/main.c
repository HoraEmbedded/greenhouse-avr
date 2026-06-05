
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

// Hardware Watchdog Timer disable (executes before main)
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) { MCUSR = 0; wdt_disable(); return; }

// ==============================================================================
// UART DRIVER (Serial Communication)
// ==============================================================================
#define BAUD 9600
#define BRC ((F_CPU/16/BAUD) - 1)

void uart_init() { UBRR0H = (BRC>>8); UBRR0L = BRC; UCSR0B = (1<<TXEN0)|(1<<RXEN0); UCSR0C = (1<<UCSZ01)|(1<<UCSZ00); }
void uart_send_char(char c) { while (!(UCSR0A & (1<<UDRE0))); UDR0 = c; }
void uart_send_string(const char* str) { while (*str) uart_send_char(*str++); }

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
void lcd_set_cursor(uint8_t col, uint8_t row) { uint8_t row_offsets[] = { 0x00, 0x40 }; lcd_send(0x80 | (col + row_offsets[row]), 0); }

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

int dht_read() {
    uint8_t i, j; volatile uint16_t timeout; 
    DDRE |= (1<<DHT_PIN); PORTE &= ~(1<<DHT_PIN); _delay_ms(20); PORTE |= (1<<DHT_PIN); _delay_us(30); DDRE &= ~(1<<DHT_PIN);
    timeout=10000; while(PINE&(1<<DHT_PIN)){if(--timeout==0)return -1;} timeout=10000; while(!(PINE&(1<<DHT_PIN))){if(--timeout==0)return -2;}
    timeout=10000; while(PINE&(1<<DHT_PIN)){if(--timeout==0)return -3;}
    for (i=0; i<5; i++) { dht_data[i] = 0; for (j=0; j<8; j++) {
        timeout=10000; while(!(PINE&(1<<DHT_PIN))){if(--timeout==0)return -4;} _delay_us(40); 
        if (PINE&(1<<DHT_PIN)) { dht_data[i] |= (1<<(7-j)); timeout=10000; while(PINE&(1<<DHT_PIN)){if(--timeout==0)return -5;} }
    }}
    if (dht_data[4] == ((dht_data[0]+dht_data[1]+dht_data[2]+dht_data[3])&0xFF)) return 0;
    return -6;
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
    
    int temperature, air_hum, dht_state;
    uint16_t raw_soil_value;
    int soil_hum_percent;
    uint8_t fan_state = 0; 
    uint8_t pump_state = 0;

    lcd_print("System Active");
    _delay_ms(2000); 
    
    sei(); 

    while (1) {
        
        if (measure_flag == 1) {
            measure_flag = 0; 
            
            dht_state = dht_read();
            raw_soil_value = adc_read();
            soil_hum_percent = (raw_soil_value * 100L) / 1023L;
            
            lcd_send(0x01, 0); // Clear LCD
            _delay_ms(2);
            
            if (dht_state == 0) {
                air_hum = (dht_data[0] << 8) | dht_data[1];
                temperature = ((dht_data[2] & 0x7F) << 8) | dht_data[3];
                if (dht_data[2] & 0x80) temperature = -temperature;
                
                // Fan Hysteresis Control
                if (temperature >= 260) fan_state = 1;
                else if (temperature <= 240) fan_state = 0;
                
                // Pump Hysteresis Control
                if (soil_hum_percent <= 30) pump_state = 1;
                else if (soil_hum_percent >= 60) pump_state = 0;
                
                // Apply relay states
                if (fan_state) PORTH |= (1 << PH6); else PORTH &= ~(1 << PH6);
                if (pump_state) PORTH |= (1 << PH5); else PORTH &= ~(1 << PH5);
                
                // Update LCD
                lcd_set_cursor(0, 0);
                lcd_print("T:"); lcd_print_int(temperature/10); lcd_print(" F:"); lcd_print(fan_state ? "ON " : "OFF");
                lcd_set_cursor(0, 1);
                lcd_print("S:"); lcd_print_int(soil_hum_percent); lcd_print("% P:"); lcd_print(pump_state ? "ON " : "OFF");
                
                // Send data via UART
                uart_send_string("T:"); uart_send_int(temperature/10);
                uart_send_string("C | Air:"); uart_send_int(air_hum/10);
                uart_send_string("% | Soil:"); uart_send_int(soil_hum_percent);
                uart_send_string("% | Fan:"); uart_send_string(fan_state ? "ON" : "OFF");
                uart_send_string(" | Pump:"); uart_send_string(pump_state ? "ON\r\n" : "OFF\r\n");
            } else {
                lcd_print("Air Sensor Err");
            }
        }
    }
    
    return 0;
}