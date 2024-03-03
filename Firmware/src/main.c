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
#include "clocked_input.pio.h"
#include "bq25792.h"

// Set up a PIO state machine to shift in serial data, sampling with an
// external clock, and push the data to the RX FIFO, 8 bits at a time.

const uint led1_pin = 7;
const uint led2_pin = 21;

#define PIN_SDA 17
#define PIN_SCL 16

#define STBY 6

const uint8_t motor_pins[] = {4, 2, 1, 3}; // AIN1, AIN2, BIN1, BIN2
const uint8_t pwm_pins[] = {0, 5, 19, 20}; // PWMA, PWMB, SERVOA, SERVOB

#define SPI_RX_PIN 25

#define PWM_CLOCK_DIVIDE 25.f // Divide the PWM clock down to a more reasonable 1MHz to even be able to do 100hz but retain precision.
#define WRAP 49999.f // calculated so it matches 100hz (original frequency/desired frequency) - 1

float tresh = 0.f;
float v_bat = 0.f;

static void alarm_callback(void) {
    v_bat = bq_getVBAT();

    if (v_bat <= tresh) {
        gpio_put(led1_pin, true);
        gpio_put(led2_pin, true);
    } else {
        gpio_put(led1_pin, false);
        gpio_put(led2_pin, false);
    }

    printf("Error present: %d\n", bq_isErrorPresent());
    printf("Battery present: %d\n", bq_isBatteryPresent());
    printf("Charge status: %s\n", bq_getChargeStatus());
    printf("IBAT: %.06f\n", bq_getIBAT());
    printf("VBAT: %.3f, Tresh: %.3f\n", v_bat, tresh);
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
    uint offset_spi = pio_add_program(pio, &clocked_input_program);
    uint sm_spi = pio_claim_unused_sm(pio, true);
    clocked_input_program_init(pio, sm_spi, offset_spi, SPI_RX_PIN);

    uint sm_i2c = pio_claim_unused_sm(pio, true);
    uint offset_i2c = pio_add_program(pio, &i2c_program);
    i2c_program_init(pio, sm_i2c, offset_i2c, PIN_SDA, PIN_SCL);
    bq_init_config(pio, sm_i2c, 18, 0);

    tresh = 3.5f * bq_getCellCount();
    v_bat = bq_getVBAT();

    uint8_t command = 0; // TODO reduce this

    uint8_t motors = 0;
    uint8_t servos = 0;

    uint8_t h1 = 0;
    uint8_t h2 = 0;

    uint16_t angle = 0;

    // Start on Wednesday 13th January 2021 11:20:00
    datetime_t t = {
        .year  = 2020,
        .month = 01,
        .day   = 13,
        .dotw  = 3, // 0 is Sunday, so 3 is Wednesday
        .hour  = 11,
        .min   = 20,
        .sec   = 50
    };

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);

    // Alarm once a minute
    datetime_t alarm = {
        .year  = -1,
        .month = -1,
        .day   = -1,
        .dotw  = -1,
        .hour  = -1,
        .min   = -1,
        .sec   = 00
    };

    rtc_set_alarm(&alarm, &alarm_callback);

    while (1){
        while (pio_sm_get_rx_fifo_level(pio, sm_spi) > 0){
            command = pio_sm_get(pio, sm_spi);
            switch (command){
                case 0x00: // Stop everything
                    gpio_put(STBY, 0);

                    for (int i = 0; i < (sizeof(motor_pins) / sizeof(motor_pins[0])) - 1; i++){ // Set all motor pins to 0
                        gpio_put(motor_pins[i], 0);
                    }

                    for (int i=0; i < (sizeof(pwm_pins) / sizeof(pwm_pins[0])) - 1; i++){ // Setup all PWM pins
                        pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[i]), false);
                        pwm_set_gpio_level(pwm_gpio_to_slice_num(pwm_pins[i]), 0); // Just for good measure
                    }

                    puts("Stop!\n");

                    break;

                case 0x01: // Enable motors [4 bits - motors A1, A2, B2, B2]
                    motors = pio_sm_get_blocking(pio, sm_spi);

                    for (int i=0; i<3; i++){
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

                    uint16_t speed_a = (int)(((uint16_t)h1 << 8) | h2) * scaler; 

                    h1 = pio_sm_get_blocking(pio, sm_spi); // Wait for second motor speed
                    h2 = pio_sm_get_blocking(pio, sm_spi);

                    uint16_t speed_b = (int)(((uint16_t)h1 << 8) | h2) * scaler; 

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

                default:
                    // printf("Unknown command: %02x", command);

                    break;
            }
            command = motors = servos = h1 = h2 = angle = 0; // Reset back to 0
        }
    }
}
