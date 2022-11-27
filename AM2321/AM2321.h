/**
 *  AM2321 (Aosong Guangzhou Electronics)
 *  Temperature and Humidity Sensor mbed library
 *  Last update : 2014/05/06
 */

#ifndef __AM2321_H__
#define __AM2321_H__

/** AM2321 (Aosong Guangzhou Electronics)
 * Temperature and Humidity Sensor mbed library
 * 
 * Example:
 * @code
 * #include "mbed.h"
 * #include "AM2321.h"
 *
 * Serial pc(USBTX, USBRX);    // Tx, Rx
 * AM2321 am2321(p28, p27);    // SDA, SCL
 *
 * int main()
 * {
 *     while(1)
 *     {
 *        if(am2321.poll())
 *         {
 *             pc.printf(":%05u,%.1f,%.1f\n"
 *                 , count++
 *                 , am2321.getTemperature()
 *                 , am2321.getHumidity()
 *             );
 *         }
 * 
 *         wait(0.5);
 *     }
 * }
 * @endcode
 */
class AM2321
{
private:
    typedef struct tagRESULT
    {
        float temperature;
        float humidity;
    }RESULT;

public:
    /** Constructor
     * @param   sda  [in]    I2C Pin name (SDA)
     * @param   scl  [in]    I2C Pin name (SCL)
     */
    AM2321(PinName sda, PinName scl);
    
    /** Read current temperature and humidity from AM2321
     * @return  result (true=success)
     */
    bool poll();

    /** Get last read temperature value
     * @return  temperature value (degress)
     */
    float getTemperature(void) const;
    
    /** Get last read humidity value
     * @return  humidity value (%RH)
     */ 
    float getHumidity(void) const;

private:
    float getLogicalValue(uint16_t regVal) const;
    uint16_t calcCRC16(const uint8_t* src, int len) const;

    I2C    _i2c;
    RESULT _result;
};


#endif // __AM2321_H__
