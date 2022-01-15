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
- PC_14 and PC_15 cannot be used (linked to micro crystal)
- PA_0 - PB_1 (left) pins are DigitalIn inputs
- PB_9 - PA_8 (right) pins are DigitalOut outputs

![board-pinout](bluepill.png)
