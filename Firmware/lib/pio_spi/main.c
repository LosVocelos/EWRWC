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
#include "clocked_input.pio.h"

// Set up a PIO state machine to shift in serial data, sampling with an
// external clock, and push the data to the RX FIFO, 8 bits at a time.

#define PWMA 0
#define PWMB 5
#define STBY 6

const uint8_t motor_pins[] = {4, 2, 1, 3}; // AIN1, AIN2, BIN1, BIN2

#define SPI_RX_PIN 25

#define PWM_CLOCK_DIVIDE 125 // Divide the PWM clock down to a more reasonable 1MHz to even be able to do 100hz but retain precision.
#define WRAP 9999 // calculated so it matches 100hz (original frequency/desired frequency) - 1

int main() {
    for (int i=0; i<3; i++){ // Set all motor pins to OUT
        gpio_init(motor_pins[i]);
        gpio_set_dir(motor_pins[i], GPIO_OUT);
    }
    gpio_init(STBY);
    gpio_set_dir(STBY, GPIO_OUT);

    gpio_set_function(PWMA, GPIO_FUNC_PWM);
    gpio_set_function(PWMB, GPIO_FUNC_PWM);

    pwm_set_clkdiv(pwm_gpio_to_slice_num(PWMA), PWM_CLOCK_DIVIDE);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(PWMB), PWM_CLOCK_DIVIDE);

    pwm_set_wrap(pwm_gpio_to_slice_num(PWMA), WRAP);
    pwm_set_wrap(pwm_gpio_to_slice_num(PWMB), WRAP);

    const float scaler = (float)WRAP/65535; // so the speed can be scaled according to the wrap

    stdio_init_all();

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &clocked_input_program);
    uint sm = pio_claim_unused_sm(pio, true);
    clocked_input_program_init(pio, sm, offset, SPI_RX_PIN);

    while (1){
        while (pio_sm_get_rx_fifo_level(pio, sm) >= 2){
            gpio_put(motor_pins[0], 1);
            gpio_put(motor_pins[1], 0);
            gpio_put(motor_pins[2], 1);
            gpio_put(motor_pins[3], 0);
            gpio_put(STBY, 1);
//             uint8_t command = pio_sm_get(pio, sm);
//             switch (command){
//                 case 0x01: // Enable motors [4 bits - pins of motors A1, A2, B2, B2]
//                     uint8_t motors = pio_sm_get_blocking(pio, sm);
//                     for (int i=0; i<3; i++){
//                         gpio_put(motor_pins[i], (motors >> i) & 1); // Go through the first 4 bits and set them to GPIO
//                     }
//                     break;
//                 case 0x10:
//
//                 default:
//                     break;
//             }
            uint8_t h1 = pio_sm_get(pio, sm);
            uint8_t h2 = pio_sm_get(pio, sm);
            uint16_t speed = ((uint16_t)h1 << 8) | h2;
            pwm_set_gpio_level(PWMA, (int)speed*scaler);
            pwm_set_gpio_level(PWMB, (int)speed*scaler);
            pwm_set_enabled(pwm_gpio_to_slice_num(PWMA), true);
            pwm_set_enabled(pwm_gpio_to_slice_num(PWMB), true);
            printf("%02x %f\n", speed, scaler);
        }
    }
}
