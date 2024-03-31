//
// Time of Flight sensor test program
//
// Copyright (c) 2017 Larry Bank
// email: bitbank@pobox.com
// Project started 7/29/2017
//
// Modified to run on Pico
// Copyright (c) 2023 Daniel Perron
// modification for pico 30 march 2023

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "tof.h" // time of flight sensor library

#define PIN_SDA 9
#define PIN_SCL 8

int main(int argc, char *argv[])
{
int i;
int iDistance;
int model, revision;

  stdio_init_all();

    // set I2C pins
    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    
    PIO pio = pio0;
    uint sm_i2c = pio_claim_unused_sm(pio, true);
    uint offset_i2c = pio_add_program(pio, &i2c_program);
    i2c_program_init(pio, sm_i2c, offset_i2c, PIN_SDA, PIN_SCL);
	sleep_ms(5000); // 5s
    printf("MAN\n");

	// For Raspberry Pi's, the I2C channel is usually 1
	// For other boards (e.g. OrangePi) it's 0
//	i = tofInit(0, 0x29, 1); // set long range mode (up to 2m)
	i = tofInit(pio, sm_i2c, 0x29, 0);
    printf("Init complete\n");

	i = tofGetModel(&model, &revision);
	printf("Model ID - %d\n", model);
	printf("Revision ID - %d\n", revision);

	if (i != 1)
	{
		return -1; // problem - quit
	}
	printf("VL53L0X device successfully opened.\n");

	for (i=0; i<120000; i++) // read values 20 times a second for 1 minute
	{
		iDistance = tofReadDistance();
		if ((iDistance < 4096) && ( iDistance > 0))// valid range?
			printf("\nDistance = %dmm\n", iDistance);
        else
            printf(".");
		sleep_ms(50); // 50ms
	}

return 0;
} /* main() */
