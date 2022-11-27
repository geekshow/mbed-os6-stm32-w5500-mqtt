/*
 *  AM2321 (Aosong Guangzhou Electronics)
 *  Temperature and Humidity Sensor mbed library
 * 
 *  Copyright (c) 2014 tomozh <tomozh@gmail.com>
 *  
 *  This software is released under the MIT License.
 *  http://opensource.org/licenses/mit-license.php
 *
 *  Last update : 2014/05/06
 */

#include "mbed.h"
#include "AM2321.h"

const int AM2321_I2C_ADDR = 0xB8;

AM2321::AM2321(PinName sda, PinName scl)
: _i2c(sda, scl)
{
    _result.temperature = 0.0f;
    _result.humidity    = 0.0f;        
}

float AM2321::getLogicalValue(uint16_t regVal) const
{
    if(regVal & 0x8000)
    {
        regVal &= ~0x8000;
        return (float)regVal / -10.0;
    }
    else
    {
        return (float)regVal / 10.0;    
    }
}

uint16_t AM2321::calcCRC16(const uint8_t* src, int len) const
{
    uint16_t crc = 0xFFFF;

    while(len--)
    {
        crc ^= *(src++);
        
        for(uint8_t i = 0; i < 8; i++)
        {
            if(crc & 0x01)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    
    return crc; 
}

bool AM2321::poll(void)
{
    bool    success = false;
    uint8_t data[8];
    
    const static uint8_t READ_REGISTER_CMD[] = {
           0x03     // Read register command
         , 0x00     // start addrress
         , 0x04     // read length
          };

    // wakeup
    _i2c.write(AM2321_I2C_ADDR, NULL, 0);

    // read data
    _i2c.write(AM2321_I2C_ADDR, (char*)READ_REGISTER_CMD, 3);
    wait_us(1600);

    if(_i2c.read(AM2321_I2C_ADDR, (char*)data, 8) == 0)
    {
        uint8_t  cmd     = data[0];
        uint8_t  dataLen = data[1];
        uint16_t humiVal = (data[2] * 256) + data[3];
        uint16_t tempVal = (data[4] * 256) + data[5];
        uint16_t recvCRC = data[6] + (data[7] * 256);
        uint16_t chkCRC  = calcCRC16(&data[0], 6);

        if(dataLen == 4)
        {
            if(recvCRC == chkCRC)
            {
                if(cmd == 0x03)
                {
                    _result.temperature = getLogicalValue(tempVal);
                    _result.humidity    = getLogicalValue(humiVal);

                    success = true;
                }
            }
        }
    }
    
    return success;
}

float AM2321::getTemperature(void) const
{
    return _result.temperature;
}

float AM2321::getHumidity(void) const
{
    return _result.humidity;
}
