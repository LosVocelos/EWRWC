#include <stdio.h>

#include "pico/stdlib.h"
//#include "pio_i2c.h"
#include "bq25792.h"

#define PIN_SDA 17
#define PIN_SCL 16

bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

int main() {
    stdio_init_all();

    printf("\nWAIT 5 SECONDS\n");

    sleep_ms(5000);

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &i2c_program);
    i2c_program_init(pio, sm, offset, PIN_SDA, PIN_SCL);
    bq_init_config(pio, sm, 18, 0);
 
    printf("\nPIO I2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }
        // Perform a 0-byte read from the probe address. The read function
        // returns a negative result NAK'd any time other than the last data
        // byte. Skip over reserved addresses.
        int result;
        if (reserved_addr(addr))
            result = -1;
        else
            result = pio_i2c_read_blocking(pio, sm, addr, NULL, 0);

        printf(result < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");

    printf("Error present: %d\n", bq_isErrorPresent());
    printf("Battery present: %d\n", bq_isBatteryPresent());
    printf("Cell cont: %i\n", bq_getCellCount());
    printf("Charge status: %s\n", bq_getChargeStatus());
    printf("Input I limit: %.06f\n", bq_getInputCurrentLimit());
    printf("Input U limit: %.06f\n", bq_getInputVoltageLimit());
    bq_setInputVoltageLimit(4.4f);
    sleep_ms(2000);
    printf("Input U limit: %.06f\n", bq_getInputVoltageLimit());
    printf("VBAT: %.06f\n", bq_getVBAT());
    printf("IBUS: %.06f\n", bq_getIBUS());
    printf("VSYS min: %.06f\n", bq_getVSYSMIN());
    printf("tt\n");
    printf("tt\n");
    return 0;
}
