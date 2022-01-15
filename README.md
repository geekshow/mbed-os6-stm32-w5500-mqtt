# Mbed OS6 STM32 IoT Ethernet Controller

IoT controller using Wiznet W5500

Main branch - simple IO control (no temp, LCD etc, just Inputs and Outputs)

Based on the BluePill (STM32F103C8) support for mbed os:

https://os.mbed.com/users/hudakz/code/mbed-os-bluepill/

## To Do

- add DS18B temp
- decrease loop timeout if required
- add i2c OLED
- add rotational encoder

## BluePill board (STM32F103C8)

Normal variant uses a mix of inputs, outputs and temperature sensing (DS18B20).

### Pins

- PC_13 is the green LED output (used to indicate online/offline status)
- PA_0 - PB_0 (left) pins are DigitalIn inputs
- PB_1 (left) is DS18B20 input (needs external 4.7k pull up)
- PB_9 - PA_8 (right) pins are DigitalOut outputs
- PC_14 and PC_15 cannot be used (linked to micro crystal)
- PB_10 is the serial output (115k) used for debug
- PB_15 - PB_12 (right) are for Wiznet SPI, PB_11 for the Wiznet reset (output)

![board-pinout](bluepill.png)
