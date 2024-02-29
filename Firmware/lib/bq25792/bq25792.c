#include "bq25792.h"

// #define BQ25792_DEBUG

#ifdef BQ25792_DEBUG
#define DEBUG_PRINTER Serial
#define DEBUG_PRINT(...)                  \
    {                                     \
        DEBUG_PRINTER.print(__VA_ARGS__); \
    }
#define DEBUG_PRINTLN(...)                  \
    {                                       \
        DEBUG_PRINTER.println(__VA_ARGS__); \
    }
#define DEBUG_PRINTF(...)                  \
    {                                      \
        DEBUG_PRINTER.printf(__VA_ARGS__); \
    }
#define DEBUG_BEGIN(...)                  \
    {                                     \
        DEBUG_PRINTER.begin(__VA_ARGS__); \
    }
#else
#define DEBUG_PRINT(...) \
    {                    \
    }
#define DEBUG_PRINTLN(...) \
    {                      \
    }
#define DEBUG_PRINTF(...) \
    {                     \
    }
#define DEBUG_BEGIN(...) \
    {                    \
    }
#endif

void bq_init_config(PIO pio, uint sm, int bcin_pin, int qon_pin) {
    bq_i2c_pio = pio;
    bq_i2c_sm = sm;
    bq_bcin_pin = bcin_pin;
    bq_qon_pin = qon_pin;
}

bool bq_flashChargeLevel(uint16_t pinToFlash, int totalDuration, uint16_t cycles)
{
    float vBat = bq_getVBAT();
    float min = bq_getVSYSMIN();
    float max = bq_getChargeVoltageLimit();

    float onTime = -1000;
    float offTime = 2000;
    totalDuration - onTime;
    while (onTime < 0 || onTime > 1000)
    {
        vBat = bq_getVBAT();
        onTime = map(vBat * 100, min * 100, max * 100, 0, totalDuration);
        DEBUG_PRINTLN("Trying");
        delay(1000);
    }
    offTime = totalDuration - onTime;
    DEBUG_PRINTF("Vbat: %.1f   Min: %.1f   Max: %.1f\n", vBat, min, max);
    DEBUG_PRINTF("ON: %.1f  OFF:%.1f\n", onTime, offTime);
    for (int i = 0; i < cycles; i++)
    {
        digitalWrite(pinToFlash, HIGH);
        delay(onTime);
        digitalWrite(pinToFlash, LOW);
        delay(offTime);
    }

    return true;
}

String bq_getChargeStatus()
{


    switch((uint8_t)bq_getChargeStatus0()){
        case 0x0:
            return String("Not Charging");
        break;
        case 0x1:
            return String("Trickle Charge");
        break;
        case 0x2:
            return String("Precharge");
        break;
        case 0x3:
            return String("Fast Charge");
        break;
        case 0x4:
            return String("Taper Charge");
        break;
        case 0x5:
            return String("Reserved");
        break;
        case 0x6:
            return String("Top Off");
        break;
        case 0x7:
            return String("Charging Done");
        break;
    }


   return String("noipe");
}

float bq_getVSYSMIN()
{
    float val = ((bq_readByte(REG00_Minimal_System_Voltage) & 0x3F) * VSYS_MIN_STEP_SIZE) + VSYS_MIN_FIXED_OFFSET;
    return val / 1000;
}

void bq_setVSYSMIN(uint8_t vsys)
{
    uint8_t reg = (vsys * 1000 - VSYS_MIN_FIXED_OFFSET) / VSYS_MIN_STEP_SIZE;
    bq_writeByte(REG00_Minimal_System_Voltage, reg);
}

uint8_t bq_getCellCount()
{
    return (int)(bq_getVSYSMIN() / 3);
}

void bq_setCellCount2(uint8_t cells)
{
    uint8_t reg = 0;
    switch (cells)
    {
    case 1:
        //(3500-Offset)/StepSize
        reg = (3500 - VSYS_MIN_FIXED_OFFSET) / VSYS_MIN_STEP_SIZE;
        bq_writeByte(REG00_Minimal_System_Voltage, reg);
        break;
    case 2:
        reg = (7000 - VSYS_MIN_FIXED_OFFSET) / VSYS_MIN_STEP_SIZE;
        bq_writeByte(REG00_Minimal_System_Voltage, reg);
        break;
    case 3:
        reg = (9000 - VSYS_MIN_FIXED_OFFSET) / VSYS_MIN_STEP_SIZE;
        bq_writeByte(REG00_Minimal_System_Voltage, reg);
        break;
    case 4:
        reg = (12000 - VSYS_MIN_FIXED_OFFSET) / VSYS_MIN_STEP_SIZE;
        bq_writeByte(REG00_Minimal_System_Voltage, reg);
        break;
    }
}

float bq_getChargeVoltageLimit()
{
    uint8_t buf[2];
    bq_readBytes(REG01_Charge_Voltage_Limit, &buf[0], 2);

    return (float)(((buf[0] & 0x07) << 8) | buf[1]) / 100;
}

void bq_setChargeVoltageLimit(float limit)
{
    uint16_t _limit = limit * 100;
    uint8_t buf[2];
    buf[0] = (uint8_t)(_limit >> 8);
    buf[1] = (uint8_t)(_limit);
    bq_writeBytes(REG01_Charge_Voltage_Limit, &buf[0], 2);
}

float bq_getChargeCurrentLimit()
{
    uint8_t buf[2];
    bq_readBytes(REG03_Charge_Current_Limit, &buf[0], 2);

    return (float)(((buf[0] & 0x01) << 8) | buf[1]) / 100;
}

void bq_setChargeCurrentLimit(float limit)
{
    uint16_t _limit = limit * 100;
    uint8_t buf[2];
    buf[0] = (uint8_t)(_limit >> 8);
    buf[1] = (uint8_t)(_limit);
    bq_writeBytes(REG03_Charge_Current_Limit, &buf[0], 2);
}

float bq_getInputVoltageLimit()
{
    uint8_t reg = bq_readByte(REG05_Input_Voltage_Limit);
    return ((float)reg / 10);
}

void bq_setInputVoltageLimit(float limit)
{
    uint8_t _limit = int(limit * 10);
    bq_writeByte(REG05_Input_Voltage_Limit, _limit);
}

float bq_getInputCurrentLimit()
{
    uint8_t buf[2];

    bq_readBytes(REG06_Input_Current_Limit, &buf[0], 2);

    return (float)(((buf[0] & 0x01) << 8) | buf[1]) / 100;
}

void bq_setInputCurrentLimit(float limit)
{
    uint16_t _limit = (int)(limit * 100);
    uint8_t buf[2];
    buf[0] = (uint8_t)(_limit >> 8);
    buf[1] = (uint8_t)(_limit);
    bq_writeBytes(REG06_Input_Current_Limit, &buf[0], 2);
}

struct precharge_control bq_getPrechargeControl()
{
    // Page 60
    // https://www.ti.com/lit/ds/symlink/bq25792.pdf?HQS=dis-dk-null-digikeymode-dsf-pf-null-wwe&ts=1685315646335

    uint8_t data = bq_readByte(REG08_Precharge_Control);
    struct precharge_control cntrl;
    cntrl.Vbat_lowV = (data & 0xC0) >> 6;
    cntrl.Iprechrg = ((float)(data & 0x3F) * 40) / 1000;
    return cntrl;
}

void bq_setPreChargeControl(struct precharge_control *cntrl)
{
    uint8_t data = (cntrl->Vbat_lowV << 6) | ((uint8_t)(cntrl->Iprechrg * 1000 / 40) & 0x3F);
    bq_writeByte(REG08_Precharge_Control, data);
}

bool bq_isPluggedIn()
{
    charger_status_0 status;
    status.raw = bq_readByte(REG1B_Charger_Status_0);
    return status.VBUS_PRESENT_STAT;
}

enum CHG_STAT bq_getChargeStatus0()
{
    enum CHG_STAT stat;
    uint8_t data = bq_readByte(REG1C_Charger_Status_1);
    data = (data >> 5) & 0x07;
    stat = static_cast<CHG_STAT>(data);
    return stat;
}

enum VBUS_STAT bq_getVBUStatus()
{
    enum VBUS_STAT stat;
    uint8_t data = bq_readByte(REG1C_Charger_Status_1);
    data = (data >> 1) & 0b00001111;
    stat = static_cast<VBUS_STAT>(data);
    return stat;
}

bool bq_isBatteryPresent()
{
    return bq_readByte(REG1D_Charger_Status_2) & 0x01;
}

bool bq_isErrorPresent()
{
    uint8_t err = bq_readByte(REG20_FAULT_Status_0) | bq_readByte(REG21_FAULT_Status_1);
    return err > 0;
}

float bq_getVBAT()
{
    bq_writeByte(REG2F_ADC_Function_Disable_0, 0b10001111);
    bq_writeByte(REG30_ADC_Function_Disable_1, 0b11111111);
    bq_writeByte(REG2E_ADC_Control, 0b10001100);

    uint8_t buf[2];
    bq_readBytes(REG3B_VBAT_ADC, &buf[0], 2);
    uint16_t v;

    return (float)(((buf[0]) << 8) | buf[1]) / 1000;
}

void bq_setCellCount(uint8_t cells)
{
    uint8_t currentConfig = bq_readByte(REG0A_Recharge_Control);
    currentConfig |= (cells << 6);
    bq_writeByte(REG0A_Recharge_Control, currentConfig);
}

float bq_twosComplementToFloat(int16_t value)
{
    int16_t mask = 0x8000; // Mask for the sign bit
    int16_t sign = value & mask;
    int16_t magnitude = value & ~mask;
    float result = static_cast<float>(magnitude);

    if (sign != 0)
    {
        // Value is negative, convert it to negative float
        result = -result;
    }

    return result;
}

float bq_getIBUS()
{
    bq_writeByte(REG2E_ADC_Control, 0b10001100);

    uint8_t buf[2];
    bq_readBytes(REG31_IBUS_ADC, &buf[0], 2);
    int16_t val = (float)(((buf[0]) << 8) | buf[1]);
    return bq_twosComplementToFloat(val);
}

void bq_getVBATReadDone()
{
}

void bq_resetPower()
{
    bq_writeByte(REG11_Charger_Control_2, 0b01000111);
}

uint8_t bq_getDeviceInfo()
{
    return bq_readByte(REG48_Part_Information);
}

void bq_reset()
{
    bq_writeByte(REG09_Termination_Control, 0b1000000);
}

void bq_readBytes(uint8_t addr, uint8_t *data, uint8_t size)
{
    int result;
    result = pio_i2c_write_blocking(bq_i2c_pio, bq_i2c_sm, DEVICEADDRESS, &addr, 1);
    DEBUG_PRINTF("readI2C reg 0x%02x\n", addr)
    result = pio_i2c_read_blocking(bq_i2c_pio, bq_i2c_sm, DEVICEADDRESS, data, size);
    for (uint8_t i = 0; i < size; ++i)
    {
        DEBUG_PRINTF(" <- data[%d]:0x%02x\n", i, data[i])
    }
}

uint8_t bq_readByte(uint8_t addr)
{
    uint8_t data;
    bq_readBytes(addr, &data, 1);
    DEBUG_PRINTF("read byte = %d\n", data)
    return data;
}

void bq_writeBytes(uint8_t addr, uint8_t *data, uint8_t size)
{
    int result;
    result = pio_i2c_write_blocking(bq_i2c_pio, bq_i2c_sm, DEVICEADDRESS, &addr, 1);
    DEBUG_PRINTF("writeI2C reg 0x%02x\n", addr)
    result = pio_i2c_write_blocking(bq_i2c_pio, bq_i2c_sm, DEVICEADDRESS, data, size);
    for (uint8_t i = 0; i < size; i++)
    {
        DEBUG_PRINTF(" -> data[%d]:0x%02x\n", i, data[i])
    }
}

void bq_writeByte(uint8_t addr, uint8_t data)
{
    DEBUG_PRINTF("write byte = %d\n", data)
    bq_writeBytes(addr, &data, 1);
}
