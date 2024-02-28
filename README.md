# EWRWC
Educational wheeled robot with camera

## Sources

These are the components (and their 3D models) I'm using in this project:

### Modules

| Part | Component | Store page | 3D model |
| ---------- | ---------- | ---------- | ---------- |
| EWRWC Add-on | Custom PCB (described below) | *None yet* | [Pi_zero_header.step](PCB/Pi_zero_header.step)
| Brain (CPU) | Raspberry pi zero 2 w | [RPishop - Zero 2 w](https://rpishop.cz/zero/4311-raspberry-pi-zero-2-w-5056561800004.html) | [GrabCAD - Zero 2 w](https://grabcad.com/library/raspberry-pi-zero-2-w-1) |
| Camera | Raspberry Pi camera V2 | [RPishop - camera v2](https://rpishop.cz/mipi-kamerove-moduly/329-raspberry-pi-kamera-modul-v2.html) | [Thingiverse - Rpi Camera v2.1](https://www.thingiverse.com/thing:2376448/files) |
| Camera cable (200 mm) | Camera Cable Standard - Mini | [RPishop - camera cable](https://rpishop.cz/mipi-kamerove-moduly/329-raspberry-pi-kamera-modul-v2.html) | []() |
| 2x DC motor | 2x GA12-N20 3V 500RPM | [Techfun - motor 3V 500RPM](https://techfun.sk/produkt/dc-motorcek-s-prevodom-rozne-typy/?attribute_pa_motor=3v-500rpm) | [GrabCAD - N20 Gearmotor](https://grabcad.com/library/dc-micro-metal-gearmotor-1) |
| Ball (support point) | Rotating ball 3PI N20 | [Techfun - 3PI N20](https://techfun.sk/produkt/gulicka-n20-pre-stavebnice-robotickych-auticok/) | []() |
| Battery holder | 18650 Li-Ion Holder | [LaskaKit - 18650 Holder](https://www.laskakit.cz/bateriovy-box-1x18650-dratove-vyvody/) | [GrabCAD - 18650 Holder](https://grabcad.com/library/18650-battery-holder-generic-1) |
| Battery | 18650 Li-Ion 2800mAh 10A | [Techfun - INR18650-28HE](https://techfun.sk/produkt/18650-bateria-tenpower-inr18650-28he-2800mah-10a/) | []() |

*These components are optional, but I have examples for them:*

| Part | Component | Store page | 3D model |
| ---------- | ---------- | ---------- | ---------- |
| 3 axis Magnetometer | I2C HMC5883l | [LaskaKit - HMC5883l](https://www.laskakit.cz/3-osy-magnetometr-a-kompas-hmc5883l/) | []() |
| Laser distance sensor | I2C VL53L0X | [LaskaKit - VL53L0X](https://www.laskakit.cz/laserovy-senzor-vzdalenosti-gy-vl53l0x-i2c/) | []() |
| Module of light intensity | I2C GY-302 BH1750 | [RPishop - GY-302](https://rpishop.cz/svetlo/2435-modul-intenzity-svetla-gy-302-bh1750.html) | []() |

*These components are optional, but I don't use them:*

| Part | Component | Store page | 3D model |
| ---------- | ---------- | ---------- | ---------- |
| Light intensity sensor | I2C VELM7700 | [LaskaKit - VELM7700](https://www.laskakit.cz/snimac-intenzity-osvetleni-veml7700--i2c/) | []() |
| Temp + humidity sensor | I2C SHT30 | [LaskaKit - SHT30](https://www.laskakit.cz/senzor-teploty-a-vlhkosti-vzduchu-sht30/) | []() |
| Blue + Yellow OLED | SPI OLED 128x64 | [LaskaKit - OLED 0.96"](https://www.laskakit.cz/oled-displej-modry-a-zluty-128x64-0-96--spi/) | []() |
| Blue + Yellow OLED | I2C OLED 128x64 | [LaskaKit - OLED 0.96"](https://www.laskakit.cz/oled-displej-modry-a-zluty-128x64-0-96--i2c/) | []() |


### PCB (EWRWC Add-on)

I previously called the board a Pi-Zero-Header (that's why the files are called that way).
But that name didn't grasp the functionality well, so I decided to call it EWRWC Add-on (I wanted to call it the HAT, but by board doesn't meet all requirements to be called that way).

List of parts used in the Add-on board:

| Part | Manufacturer | Component | Store page | Documentation |
| ---------- | ---------- | ---------- | ---------- | ---------- |
| Microcontroller | Raspberry Pi | RP2040 | [LCSC - RP2040](https://www.lcsc.com/product-detail/Microcontroller-Units-MCUs-MPUs-SOCs_Raspberry-Pi-RP2040_C2040.html) | [rp2040-datasheet.pdf](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf) |
| MCU Flash memory | Winbond | Q25W128JV | [JLCPCB - Q25W128JVSIQ](https://jlcpcb.com/partdetail/WinbondElec-W25Q128JVSIQ/C97521) | [W25Q128JV.pdf](https://www.winbond.com/resource-files/W25Q128JV%20RevI%2008232021%20Plus.pdf) |
| MCU Crystal | YXC | X322512MSB4SI | [JLCPCB - X322512MSB4SI](https://jlcpcb.com/partdetail/Yxc-X322512MSB4SI/C9002) | [YSX321SL.pdf](https://image.seapx.com/mall/yangxin/3/20231030/YSX321SL-687795.pdf) |
| Battery management | Texas Instruments | BQ25792 | [LCSC - BQ25792](https://www.lcsc.com/product-detail/Battery-Management-ICs_Texas-Instruments-BQ25792RQMR_C2862876.html) | [bq25792.pdf](https://www.ti.com/lit/ds/symlink/bq25792.pdf) |
| 5V / 3V3 Regulator | Texas Instruments | TPS63070 | [LCSC - TPS63070](https://www.lcsc.com/product-detail/DC-DC-Converters_Texas-Instruments-TPS63070RNMR_C109322.html) | [tps63070.pdf](https://www.ti.com/lit/ds/symlink/tps63070.pdf) |
| Motor Driver (2 H) | Toshiba | TB6612FNG | [LCSC - TB6612FNG](https://www.lcsc.com/product-detail/Motor-Driver-ICs_TOSHIBA-TB6612FNG-O-C-8-EL_C88224.html) | [TB6612FNG.pdf](https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG) |

There are much more parts used in this project (inductors, buttons, connectors, LEDs and a bunch of capacitors and resistors), but only those in the table are crucialfor the functionality and choosing the rest of the parts.

--------------------------------------------------

TODO:

- [x] Gather all sources
- [x] Design the main PCB
- [x] Complete the PCB
- [ ] Design chassis
- [ ] Print and ready the chassis
- [ ] Complete the hardware
- [ ] Write the software
- [ ] Write the framework
- [ ] Add some use for the camera
- [ ] Write documentation
- [ ] Finalize
- [ ] Play with it :)
