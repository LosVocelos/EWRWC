/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "spi_slave.pio.h"
#include "bq25792.h"
#include "ssd1306.h"
#include "tof.h"

// Set up a PIO state machine to shift in serial data, sampling with an
// external clock, and push the data to the RX FIFO, 8 bits at a time.

const uint led1_pin = 7;
const uint led2_pin = 21;

#define PIN_SDA_BQ 17
#define PIN_SCL_BQ 16

#define PIN_SDA 9
#define PIN_SCL 8

#define SPI_RX_PIN 25
#define SPI_TX_PIN 24

#define STBY 6

const uint8_t motor_pins[] = {4, 2, 1, 3}; // AIN1, AIN2, BIN1, BIN2
const uint8_t pwm_pins[] = {0, 5, 19, 20}; // PWMA, PWMB, SERVOA, SERVOB

#define PWM_CLOCK_DIVIDE 25.f // Divide the PWM clock down to a more reasonable 1MHz to even be able to do 100hz but retain precision.
#define WRAP 49999.f // calculated so it matches 100hz (original frequency/desired frequency) - 1

int tresh = 0;
int v_bat = 0;
int i_bat = 0;
uint8_t bat_stat = 0x0;

char buff [32];

int iDistance;

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT_S(str, data) {                  \
    fputs(str, stdout);                             \
    fputs(data, stdout);                            \
    fputs("\n", stdout);                            \
}
#define DEBUG_PRINT_I(str, data, base) {            \
    itoa(data, buff, base);                         \
    fputs(str, stdout);                             \
    fputs(buff, stdout);                            \
    fputs("\n", stdout);                            \
}
#define DEBUG_PRINT_F(str, data, format) {          \
    fputs(str, stdout);                             \
    snprintf(buff, sizeof buff, format, data);      \
    fputs(buff, stdout);                            \
    fputs("\n", stdout);                            \
}
#else
#define DEBUG_PRINT_S(str, data) {}
#define DEBUG_PRINT_I(str, data, base) {}
#define DEBUG_PRINT_F(str, data, format) {}
#endif


// Start on Wednesday 13th January 2021 11:20:00
datetime_t t = {
    .year  = 1970,
    .month = 01,
    .day   = 01,
    .dotw  = 4, // 0 is Sunday, so 3 is Wednesday
    .hour  = 0,
    .min   = 0,
    .sec   = 0
};

// Alarm once a minute
datetime_t alarm = {
    .year  = -1,
    .month = -1,
    .day   = -1,
    .dotw  = -1,
    .hour  = -1,
    .min   = -1,
    .sec   = 05
};

ssd1306_t disp;

static void alarm_callback(void) {
    v_bat = bq_getVBAT();
    i_bat = bq_getIBAT();
    bat_stat = bq_getChargeStatus0();

    if (v_bat <= tresh) {
        gpio_put(led1_pin, true);
        gpio_put(led2_pin, true);
    } else {
        gpio_put(led1_pin, false);
        gpio_put(led2_pin, false);
    }
    ssd1306_clear_square(&disp, 0, 0, 128, 16);
    ssd1306_draw_string(&disp, 0, 0, 1, "BAT:");
    itoa(v_bat, buff, 10);
    ssd1306_draw_string(&disp, 30, 0, 1, buff);
    ssd1306_draw_string(&disp, 62, 0, 1, "mV");
    itoa(i_bat, buff, 10);
    ssd1306_draw_string(&disp, 80, 0, 1, buff);
    ssd1306_draw_string(&disp, 116, 0, 1, "mA");
    ssd1306_draw_string(&disp, 30, 8, 1, bq_getChargeStatus());

    DEBUG_PRINT_I("Error present: ", bq_isErrorPresent(), 2);
    DEBUG_PRINT_I("Battery present: ", bq_isBatteryPresent(), 2);
    DEBUG_PRINT_S("Charge status: ", bq_getChargeStatus());
    DEBUG_PRINT_I("IBAT: ", i_bat, 10);
    DEBUG_PRINT_I("VBAT: ", v_bat, 10);
    DEBUG_PRINT_I("Tresh: ", tresh, 10);
    DEBUG_PRINT_S("--------------------", "\n");

    rtc_set_datetime(&t);
}

int main() {
    stdio_init_all();

    for (int i=0; i < 4; i++){ // Set all motor pins to OUT
        gpio_init(motor_pins[i]);
        gpio_set_dir(motor_pins[i], GPIO_OUT);
    }

    gpio_init(STBY); 
    gpio_set_dir(STBY, GPIO_OUT);
    gpio_init(led1_pin);
    gpio_set_dir(led1_pin, GPIO_OUT);
    gpio_init(led2_pin);
    gpio_set_dir(led2_pin, GPIO_OUT);
    gpio_put(led1_pin, false);
    gpio_put(led2_pin, false);  

    for (int i=0; i < 4; i++){ // Setup all PWM pins
        gpio_set_function(pwm_pins[i], GPIO_FUNC_PWM);
        pwm_set_clkdiv(pwm_gpio_to_slice_num(pwm_pins[i]), PWM_CLOCK_DIVIDE);
        pwm_set_wrap(pwm_gpio_to_slice_num(pwm_pins[i]), WRAP);
    }

    const float scaler = (float)WRAP/65535; // so the speed can be scaled according to the wrap

    PIO pio = pio0;
    uint offset_spi = pio_add_program(pio, &spi_slave_program);
    uint sm_spi = pio_claim_unused_sm(pio, true);
    spi_slave_program_init(pio, sm_spi, offset_spi, SPI_RX_PIN, SPI_TX_PIN);
    
    uint sm_i2c_bq = pio_claim_unused_sm(pio, true);
    uint offset_i2c_bq = pio_add_program(pio, &i2c_program);
    i2c_program_init(pio, sm_i2c_bq, offset_i2c_bq, PIN_SDA_BQ, PIN_SCL_BQ);
    bq_init_config(pio, sm_i2c_bq, 18, 0);

    tresh = 3600 * bq_getCellCount();
    v_bat = bq_getVBAT();

    PIO pio_i2c = pio1;
    uint sm_i2c = pio_claim_unused_sm(pio_i2c, true);
    uint offset_i2c = pio_add_program(pio_i2c, &i2c_program);
    i2c_program_init(pio_i2c, sm_i2c, offset_i2c, PIN_SDA, PIN_SCL);

	tofInit(pio_i2c, sm_i2c, 0x29, 0);

    disp.external_vcc=false;
    ssd1306_init(&disp, 128, 64, 0x3C, pio_i2c, sm_i2c);
    ssd1306_clear(&disp);
    ssd1306_show(&disp);

    uint8_t command = 0; // TODO reduce this
    uint32_t data_out;
    int i_loop = 0;

    uint8_t motors = 0;
    uint8_t servos = 0;

    uint8_t h1 = 0;
    uint8_t h2 = 0;
    uint8_t h3 = 0;
    uint8_t h4 = 0;

    uint16_t angle = 0;

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);

    rtc_set_alarm(&alarm, &alarm_callback);

    pio_sm_clear_fifos(pio, sm_spi);  // Clear buffers on start
    sleep_ms(5000);

    while (1){
        ssd1306_clear_square(&disp, 0, 16, 128, 48);

        // VL53L0X handeling
        ssd1306_draw_string(&disp, 14, 20, 1, "Distance =     mm");
        iDistance = tofReadDistance();
		if ((iDistance < 4096) && ( iDistance > 0)){// valid range?
            itoa(iDistance, buff, 10);
            ssd1306_draw_string(&disp, 80, 20, 1, buff);
        }
        else{
            ssd1306_draw_string(&disp, 80, 20, 1, "....");
        }
        //DEBUG_PRINT_I("Distance (mm) = ", iDistance, 10);

        while (pio_sm_get_rx_fifo_level(pio, sm_spi) > 1){
            command = pio_sm_get(pio, sm_spi);
            DEBUG_PRINT_I("cmd:", command, 16);
            
            // pio_sm_clear_fifos(pio, sm_spi);
            switch (command){
                case 0xFF: // Stop everything
                    gpio_put(STBY, 0);

                    for (int i = 0; i < (sizeof(motor_pins) / sizeof(motor_pins[0])) - 1; i++){ // Set all motor pins to 0
                        gpio_put(motor_pins[i], 0);
                    }

                    for (int i=0; i < (sizeof(pwm_pins) / sizeof(pwm_pins[0])) - 1; i++){ // Setup all PWM pins
                        pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[i]), false);
                        pwm_set_gpio_level(pwm_gpio_to_slice_num(pwm_pins[i]), 0); // Just for good measure
                    }

                    DEBUG_PRINT_S("Stop!", "");

                    break;

                case 0x01: // Enable motors [4 bits - motors A1, A2, B2, B2]
                    motors = pio_sm_get_blocking(pio, sm_spi);
                    DEBUG_PRINT_I("motors:", motors, 2);

                    for (int i=0; i<4; i++){
                        gpio_put(motor_pins[i], (motors >> i) & 1); // Go through the first 4 bits and set them to GPIO
                    }

                    gpio_put(STBY, 1);

                    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[0]), true);
                    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[1]), true);

                    break;
                
                case 0x02: // Enable servos [2 bits - servos SERVOA, SERVOB]
                    servos = pio_sm_get_blocking(pio, sm_spi);

                    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[2]), servos & 1);
                    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[3]), (servos >> 1) & 1);

                    break;

                case 0x10: // Set speed of motors [4 bytes -> 2 uint16_t speed]
                    h1 = pio_sm_get_blocking(pio, sm_spi); // Wait for one motor speed
                    h2 = pio_sm_get_blocking(pio, sm_spi);

                    h3 = pio_sm_get_blocking(pio, sm_spi); // Wait for second motor speed
                    h4 = pio_sm_get_blocking(pio, sm_spi);
                    
                    uint16_t speed_a = (int)(((uint16_t)h1 << 8) | h2) * scaler;
                    uint16_t speed_b = (int)(((uint16_t)h3 << 8) | h4) * scaler;

                    DEBUG_PRINT_I("speed_a:", speed_a, 10);
                    DEBUG_PRINT_I("speed_b:", speed_b, 10);

                    pwm_set_gpio_level(pwm_pins[0], speed_a); // Set speed
                    pwm_set_gpio_level(pwm_pins[1], speed_b);

                    // printf("Speed A:%02x Speed B:%02x\n", speed_a, speed_b);
                    
                    break;

                case 0x20: // Set speed angle of SERVOA (it's a duty cycle) [2 bytes -> uint16_t speed]
                    h1 = pio_sm_get_blocking(pio, sm_spi); // Wait for angle
                    h2 = pio_sm_get_blocking(pio, sm_spi);

                    angle = (int)(((uint16_t)h1 << 8) | h2) * scaler;

                    pwm_set_gpio_level(pwm_pins[2], angle); // Set speed

                    // printf("Servo A:%02x\n", angle);
                    
                    break;
                
                case 0x21: // Set speed angle of SERVOB (it's a duty cycle) [2 bytes -> uint16_t speed]
                    h1 = pio_sm_get_blocking(pio, sm_spi); // Wait for angle
                    h2 = pio_sm_get_blocking(pio, sm_spi);

                    angle = (int)(((uint16_t)h1 << 8) | h2) * scaler;

                    pwm_set_gpio_level(pwm_pins[3], angle); // Set speed

                    // printf("Servo B:%02x\n", angle);
                    
                    break;

                case 0x6C: // Read data from charger
                    pio_sm_clear_fifos(pio, sm_spi);  // Clear buffers on start
                    pio_sm_put_blocking(pio, sm_spi, v_bat<<16);
                    pio_sm_put_blocking(pio, sm_spi, v_bat<<24);

                    pio_sm_put_blocking(pio, sm_spi, i_bat<<16);
                    pio_sm_put_blocking(pio, sm_spi, i_bat<<24);

                    pio_sm_put_blocking(pio, sm_spi, bat_stat<<16);
                    pio_sm_put_blocking(pio, sm_spi, bat_stat<<24);

                    break;

                case 0x29: // Read data from vl53l0x
                    pio_sm_clear_fifos(pio, sm_spi);  // Clear buffers on start
                    pio_sm_put_blocking(pio, sm_spi, iDistance<<16);
                    pio_sm_put_blocking(pio, sm_spi, iDistance<<24);

                    break;

                default:
                    // printf("Unknown command: %02x", command);

                    break;
            }

            //pio_sm_clear_fifos(pio, sm_spi);  // Clear buffers on start
            command = motors = servos = h1 = h2 = angle = 0; // Reset back to 0
            DEBUG_PRINT_S("--------------------", "\n");
        }
        ssd1306_show(&disp);
    }
}
