/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "spi_slave.pio.h"

// Set up a PIO state machine to shift in serial data, sampling with an
// external clock, and push the data to the RX FIFO, 8 bits at a time.

#define STBY 6

const uint8_t motor_pins[] = {4, 2, 1, 3}; // AIN1, AIN2, BIN1, BIN2
const uint8_t pwm_pins[] = {0, 5, 19, 20}; // PWMA, PWMB, SERVOA, SERVOB

#define SPI_RX_PIN 25
#define SPI_TX_PIN 24

#define PWM_CLOCK_DIVIDE 25.f // Divide the PWM clock down to a more reasonable 1MHz to even be able to do 100hz but retain precision.
#define WRAP 49999.f // calculated so it matches 100hz (original frequency/desired frequency) - 1

int main() {
    stdio_init_all();

    for (int i=0; i < 4; i++){ // Set all motor pins to OUT
        gpio_init(motor_pins[i]);
        gpio_set_dir(motor_pins[i], GPIO_OUT);
    }

    gpio_init(STBY); 
    gpio_set_dir(STBY, GPIO_OUT);

    for (int i=0; i < 4; i++){ // Setup all PWM pins
        gpio_set_function(pwm_pins[i], GPIO_FUNC_PWM);
        pwm_set_clkdiv(pwm_gpio_to_slice_num(pwm_pins[i]), PWM_CLOCK_DIVIDE);
        pwm_set_wrap(pwm_gpio_to_slice_num(pwm_pins[i]), WRAP);
    }

    const float scaler = (float)WRAP/65535; // so the speed can be scaled according to the wrap

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &spi_slave_program);
    uint sm = pio_claim_unused_sm(pio, true);
    spi_slave_program_init(pio, sm, offset, SPI_RX_PIN, SPI_TX_PIN);

    uint8_t command = 0; // TODO reduce this

    uint8_t motors = 0;
    uint8_t servos = 0;

    uint8_t h1 = 0;
    uint8_t h2 = 0;

    uint16_t angle = 0;

    sleep_ms(5000);
    while (1){
        if (pio_sm_is_tx_fifo_empty(pio, sm)){
            pio_sm_put(pio, sm, 0xFFFF);
            sleep_ms(100);
            printf("sent data:%d", 0xFFFF);
        }
        while (pio_sm_get_rx_fifo_level(pio, sm) > 0){
            command = pio_sm_get(pio, sm);
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
                    motors = pio_sm_get_blocking(pio, sm);

                    for (int i=0; i<3; i++){
                        gpio_put(motor_pins[i], (motors >> i) & 1); // Go through the first 4 bits and set them to GPIO
                    }

                    gpio_put(STBY, 1);

                    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[0]), true);
                    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[1]), true);

                    break;
                
                case 0x02: // Enable servos [2 bits - servos SERVOA, SERVOB]
                    servos = pio_sm_get_blocking(pio, sm);

                    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[2]), servos & 1);
                    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_pins[3]), (servos >> 1) & 1);

                    break;

                case 0x10: // Set speed of motors [4 bytes -> 2 uint16_t speed]
                    h1 = pio_sm_get_blocking(pio, sm); // Wait for one motor speed
                    h2 = pio_sm_get_blocking(pio, sm);

                    uint16_t speed_a = (int)(((uint16_t)h1 << 8) | h2) * scaler; 

                    h1 = pio_sm_get_blocking(pio, sm); // Wait for second motor speed
                    h2 = pio_sm_get_blocking(pio, sm);

                    uint16_t speed_b = (int)(((uint16_t)h1 << 8) | h2) * scaler; 

                    pwm_set_gpio_level(pwm_pins[0], speed_a); // Set speed
                    pwm_set_gpio_level(pwm_pins[1], speed_b);

                    // printf("Speed A:%02x Speed B:%02x\n", speed_a, speed_b);
                    
                    break;

                case 0x20: // Set speed angle of SERVOA (it's a duty cycle) [2 bytes -> uint16_t speed]
                    h1 = pio_sm_get_blocking(pio, sm); // Wait for angle
                    h2 = pio_sm_get_blocking(pio, sm);

                    angle = (int)(((uint16_t)h1 << 8) | h2) * scaler;

                    pwm_set_gpio_level(pwm_pins[2], angle); // Set speed

                    // printf("Servo A:%02x\n", angle);
                    
                    break;
                
                case 0x21: // Set speed angle of SERVOB (it's a duty cycle) [2 bytes -> uint16_t speed]
                    h1 = pio_sm_get_blocking(pio, sm); // Wait for angle
                    h2 = pio_sm_get_blocking(pio, sm);

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
