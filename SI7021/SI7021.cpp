/**
 * @brief       SI7021.h
 * @details     I2C HUMIDITY AND TEMPERATURE SENSOR.
 *              Header file.
 *
 *
 * @return      NA
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018    The ORIGIN
 * @pre         N/A.
 * @warning     N/A
 * @pre         This code belongs to AqueronteBlog ( http://unbarquero.blogspot.com ).
 */

#include "SI7021.h"


SI7021::SI7021 ( PinName sda, PinName scl, uint32_t addr, uint32_t freq )
    : _i2c          ( sda, scl )
    , _SI7021_Addr  ( addr )
{
    _i2c.frequency( freq );
}


SI7021::~SI7021()
{
}



/**
 * @brief       SI7021_Conf ( SI7021_measurement_resolution_t , SI7021_heater_t )
 *
 * @details     It configures the device: resolution and heater.
 *
 * @param[in]    myResolution:      Resolution for RH and temperature.
 * @param[in]    myHeater:          Enable/Disable heater.
 *
 * @param[out]   N/A.
 *
 *
 * @return       Status of SI7021_Conf.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         Except where noted, reserved register bits will always read back as “1,” and are not affected by write operations. For
 *              future compatibility, it is recommended that prior to a write operation, registers should be read. Then the values read
 *              from the RSVD bits should be written back unchanged during the write operation
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_Conf ( SI7021_measurement_resolution_t myResolution, SI7021_heater_t myHeater )
{
    char      cmd[]    =    { SI7021_READ_RH_T_USER_REGISTER_1, 0 };
    uint32_t  aux      =    0;



    // Read USER REGISTER to mask it
    aux = _i2c.write ( _SI7021_Addr, &cmd[0], 1, true );
    aux = _i2c.read  ( _SI7021_Addr, &cmd[1], 1 );


    // Update USER REGISTER according to user requirements
    // Resolution
    cmd[1]  &= ~SI7021_RESOLUTION_MASK;
    cmd[1]  |=  myResolution;

    // Hater
    cmd[1]  &= ~SI7021_HTRE_MASK;
    cmd[1]  |=  myHeater;



    // Update USER REGISTER
    cmd[0]   =   SI7021_WRITE_RH_T_USER_REGISTER_1;
    aux      =   _i2c.write ( _SI7021_Addr, &cmd[0], 2, false );




    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_SoftReset ( void )
 *
 * @details     It resets the device by software.
 *
 * @param[in]    N/A
 *
 * @param[out]   N/A.
 *
 *
 * @return       Status of SI7021_SoftReset.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         N/A
 * @warning     The user MUST keep in mind that the device will be
 *              available after 5ms ( typically, 15ms maximum )
 *              The user MUST take care of this delay!.
 */
SI7021::SI7021_status_t  SI7021::SI7021_SoftReset ( void )
{
    char      cmd    =    SI7021_RESET;
    uint32_t  aux    =    0;


    aux      =   _i2c.write ( _SI7021_Addr, &cmd, 1, false );

    // The user needs to wait for certain period of time before communicating
    // with the device again.


    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_GetElectronicSerialNumber ( SI7021_vector_data_t* )
 *
 * @details     It gets the electronic serial number.
 *
 * @param[in]    N/A
 *
 * @param[out]   mySerialNumber:     The Electronic Serial Number.
 *
 *
 * @return       Status of SI7021_GetElectronicSerialNumber.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         This function does NOT check the CRC
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_GetElectronicSerialNumber   ( SI7021_vector_data_t* mySerialNumber )
{
    char      cmd[]    =    { SI7021_READ_ELECTRONIC_ID_FIRST_BYTE_CMD1, SI7021_READ_ELECTRONIC_ID_FIRST_BYTE_CMD2, 0, 0, 0, 0, 0, 0 };
    uint32_t  aux      =    0;


    // Read Electronic Serial Number. First Access
    aux = _i2c.write ( _SI7021_Addr, &cmd[0], 2, true );
    aux = _i2c.read  ( _SI7021_Addr, &cmd[0], sizeof( cmd )/sizeof( cmd[0] ) );


    mySerialNumber->ElectronicSerialNumber_MSB   =   ( ( cmd[0] << 24 ) | ( cmd[2] << 16 ) | ( cmd[4] << 8 ) | cmd[6] );


    // Read Electronic Serial Number. Second Access
    cmd[0]   =   SI7021_READ_ELECTRONIC_ID_SECOND_BYTE_CMD1;
    cmd[1]   =   SI7021_READ_ELECTRONIC_ID_SECOND_BYTE_CMD2;

    aux = _i2c.write ( _SI7021_Addr, &cmd[0], 2, true );
    aux = _i2c.read  ( _SI7021_Addr, &cmd[0], sizeof( cmd )/sizeof( cmd[0] ) );


    mySerialNumber->ElectronicSerialNumber_LSB   =   ( ( cmd[0] << 24 ) | ( cmd[2] << 16 ) | ( cmd[4] << 8 ) | cmd[6] );




    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_GetFirmwareRevision ( SI7021_vector_data_t* )
 *
 * @details     It gets the firmware revision.
 *
 * @param[in]    N/A
 *
 * @param[out]   myFirmwareRevision: The firmware revision.
 *
 *
 * @return       Status of SI7021_GetFirmwareRevision.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         N/A.
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_GetFirmwareRevision ( SI7021_vector_data_t* myFirmwareRevision )
{
    char      cmd[]    =    { SI7021_READ_FIRMWARE_VERSION_CMD1, SI7021_READ_FIRMWARE_VERSION_CMD2 };
    uint32_t  aux      =    0;


    // Read the firmware revision
    aux = _i2c.write ( _SI7021_Addr, &cmd[0], 2, true );
    aux = _i2c.read  ( _SI7021_Addr, &cmd[0], 1 );


    myFirmwareRevision->FirmwareRevision   =   cmd[0];




    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_SetHeaterCurrent ( char )
 *
 * @details     It sets the heater current.
 *
 * @param[in]    myHeaterCurrent:   New heater current value
 *
 * @param[out]   N/A.
 *
 *
 * @return       Status of SI7021_SetHeaterCurrent.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         N/A.
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_SetHeaterCurrent ( char myHeaterCurrent )
{
    char      cmd[]    =    { SI7021_READ_HEATER_CONTROL_REGISTER, 0 };
    uint32_t  aux      =    0;

    
    // myHeaterCurrent MUST be from 0 ( 0x00 ) to 15 ( 0x0F )
    if ( myHeaterCurrent <= 15 ) {
        // Read the heater control register
        aux = _i2c.write ( _SI7021_Addr, &cmd[0], 1, true );
        aux = _i2c.read  ( _SI7021_Addr, &cmd[1], 1 );


        // Mask the data
        cmd[1]  &=  0xF0;

        // Update the value
        cmd[0]  =   SI7021_WRITE_HEATER_CONTROL_REGISTER;
        cmd[1] |=   myHeaterCurrent;


        aux = _i2c.write ( _SI7021_Addr, &cmd[0], 2, false );
    } else
        return   SI7021_FAILURE;




    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_TriggerTemperature ( SI7021_master_mode_t )
 *
 * @details     It performs a new temperature measurement.
 *
 * @param[in]    myMode:             HOLD/NO HOLD mode.
 *
 * @param[out]   N/A
 *
 *
 * @return       Status of SI7021_TriggerTemperature.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         N/A.
 * @warning     The user MUST respect the total conversion time.
 *              14-bit temperature: 7ms   ( 10.8ms max )
 *              13-bit temperature: 4ms   (  6.2ms max )
 *              12-bit temperature: 2.4ms (  3.8ms max )
 *              11-bit temperature: 1.5ms (  2.4ms max )
 */
SI7021::SI7021_status_t  SI7021::SI7021_TriggerTemperature ( SI7021_master_mode_t myMode  )
{
    char         cmd        =    0;
    bool         myI2C_stop =    false;
    uint32_t     aux        =    0;



    // Check the mode if it is HOLD MASTER MODE, then not generate a stop bit
    if ( myMode == SI7021_HOLD_MASTER_MODE ) {
        cmd         =    SI7021_MEASURE_TEMPERATURE_HOLD_MASTER_MODE;
        myI2C_stop  =    true;
    } else {
        cmd         =    SI7021_MEASURE_TEMPERATURE_NO_HOLD_MASTER_MODE;
        myI2C_stop  =    false;
    }



    aux = _i2c.write ( _SI7021_Addr, &cmd, 1, myI2C_stop );


    // NOTE: The user has to respect the total conversion time!


    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_ReadTemperature ( SI7021_vector_data_t* )
 *
 * @details     It reads the temperature.
 *
 * @param[in]    myMode:             HOLD/NO HOLD mode.
 *
 * @param[out]   myTemperature:      The current temperature
 *
 *
 * @return       Status of SI7021_ReadTemperature.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         SI7021_TriggerTemperature MUST be called first. This function does NOT
 *              read CRC byte.
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_ReadTemperature ( SI7021_vector_data_t* myTemperature )
{
    char      cmd[]      =    { 0, 0 };
    uint32_t  aux        =    0;




    aux = _i2c.read ( _SI7021_Addr, &cmd[0], sizeof( cmd )/sizeof( cmd[0] ) );


    // Check if the is a valid data
    if ( ( cmd[1] & 0x02 )  ==  0x00 ) {
        // Parse the data
        myTemperature->Temperature   =   ( ( cmd[0] << 8 ) | cmd[1] );
        myTemperature->Temperature  *=   175.72;
        myTemperature->Temperature  /=   65536.0;
        myTemperature->Temperature  -=   46.85;
    } else
        return   SI7021_FAILURE;





    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_ReadRawTemperatureFromRH ( SI7021_vector_data_t* )
 *
 * @details     It reads the raw temperature.
 *
 * @param[in]    N/A.
 *
 * @param[out]   myTemperature:      The current temperature
 *
 *
 * @return       Status of SI7021_ReadRawTemperatureFromRH.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         SI7021_TriggerHumidity MUST be called first.
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_ReadRawTemperatureFromRH ( SI7021_vector_data_t* myTemperature )
{
    char      cmd[]      =    { SI7021_READ_TEMPERATURE_VALUE_FROM_PREVIOUS_RH_MEASUREMENT, 0 };
    uint32_t  aux        =    0;



    aux = _i2c.write ( _SI7021_Addr, &cmd[0], 1, true );
    aux = _i2c.read  ( _SI7021_Addr, &cmd[0], 2 );


    // Check if the is a valid data
    if ( ( cmd[1] & 0x02 )  ==  0x00 ) {
        // Parse the data
        myTemperature->Temperature   =   ( ( cmd[0] << 8 ) | cmd[1] );
    } else
        return   SI7021_FAILURE;




    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_ReadTemperatureFromRH ( SI7021_vector_data_t* )
 *
 * @details     It reads the temperature.
 *
 * @param[in]    N/A.
 *
 * @param[out]   myTemperature:      The current temperature
 *
 *
 * @return       Status of SI7021_ReadTemperatureFromRH.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         SI7021_TriggerHumidity MUST be called first.
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_ReadTemperatureFromRH ( SI7021_vector_data_t* myTemperature )
{
    char      cmd[]      =    { SI7021_READ_TEMPERATURE_VALUE_FROM_PREVIOUS_RH_MEASUREMENT, 0 };
    uint32_t  aux        =    0;



    aux = _i2c.write ( _SI7021_Addr, &cmd[0], 1, true );
    aux = _i2c.read  ( _SI7021_Addr, &cmd[0], 2 );


    // Check if the is a valid data
    if ( ( cmd[1] & 0x02 )  ==  0x00 ) {
        // Parse the data
        myTemperature->Temperature   =   ( ( cmd[0] << 8 ) | cmd[1] );
        myTemperature->Temperature  *=   175.72;
        myTemperature->Temperature  /=   65536.0;
        myTemperature->Temperature  -=   46.85;
    } else
        return   SI7021_FAILURE;




    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_TriggerHumidity ( SI7021_master_mode_t )
 *
 * @details     It performs a new relative humidity measurement.
 *
 * @param[in]    myMode:             HOLD/NO HOLD MASTER mode.
 *
 * @param[out]   N/A
 *
 *
 * @return       Status of SI7021_TriggerHumidity.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         Initiating a RH measurement will also automatically initiate a temperature measurement.
 *              The total conversion time will be tCONV(RH) + tCONV(T).
 * @warning     The user MUST respect the total conversion time.
 *              12-bit RH: 10ms   ( 12ms max )
 *              11-bit RH: 5.8ms  (  7ms max )
 *              10-bit RH: 3.7ms  (  4.5ms max )
 *               8-bit RH: 2.6ms  (  3.1ms max )
 */
SI7021::SI7021_status_t  SI7021::SI7021_TriggerHumidity ( SI7021_master_mode_t myMode )
{
    char         cmd        =    myMode;
    uint32_t     aux        =    0;
    bool         myI2C_stop =    false;



    // Check the mode if it is HOLD MASTER MODE, then not generate a stop bit
    if ( myMode == SI7021_HOLD_MASTER_MODE ) {
        cmd         =    SI7021_MEASURE_RELATIVE_HUMIDITY_HOLD_MASTER_MODE;
        myI2C_stop  =    true;
    } else {
        cmd         =    SI7021_MEASURE_RELATIVE_HUMIDITY_NO_HOLD_MASTER_MODE;
        myI2C_stop  =    false;
    }


    aux = _i2c.write ( _SI7021_Addr, &cmd, 1, myI2C_stop );


    // NOTE: The user has to respect the total conversion time!


    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_ReadHumidity ( SI7021_vector_data_t* )
 *
 * @details     It reads the relative humidity.
 *
 * @param[in]    N/A.
 *
 * @param[out]   myHumidity:         The current HUMIDITY
 *
 *
 * @return       Status of SI7021_ReadHumidity.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         SI7021_TriggerHumidity MUST be called first. This function does NOT
 *              read CRC byte.
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_ReadHumidity ( SI7021_vector_data_t* myHumidity )
{
    char      cmd[]      =    { 0, 0 };
    uint32_t  aux        =    0;



    aux = _i2c.read ( _SI7021_Addr, &cmd[0], sizeof( cmd )/sizeof( cmd[0] ) );


    // Check if the is a valid data
    if ( ( cmd[1] & 0x03 )  ==  0x02 ) {
        // Parse the data
        myHumidity->RelativeHumidity   =   ( ( cmd[0] << 8 ) | cmd[1] );
        myHumidity->RelativeHumidity  *=   125.0;
        myHumidity->RelativeHumidity  /=   65536.0;
        myHumidity->RelativeHumidity  -=   6.0;
    } else
        return   SI7021_FAILURE;




    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_ReadRawHumidity ( SI7021_vector_data_t* )
 *
 * @details     It reads the raw relative humidity.
 *
 * @param[in]    N/A.
 *
 * @param[out]   myHumidity:         The current HUMIDITY
 *
 *
 * @return       Status of SI7021_ReadRawHumidity.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         SI7021_TriggerHumidity MUST be called first. This function does NOT
 *              read CRC byte.
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_ReadRawHumidity ( SI7021_vector_data_t* myHumidity )
{
    char      cmd[]      =    { 0, 0 };
    uint32_t  aux        =    0;



    aux = _i2c.read ( _SI7021_Addr, &cmd[0], sizeof( cmd )/sizeof( cmd[0] ) );


    // Check if the is a valid data
    if ( ( cmd[1] & 0x03 )  ==  0x02 ) {
        // Parse the data
        myHumidity->RelativeHumidity   =   ( ( cmd[0] << 8 ) | cmd[1] );
    } else
        return   SI7021_FAILURE;




    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}



/**
 * @brief       SI7021_GetBatteryStatus ( SI7021_vector_data_t* )
 *
 * @details     It reads the raw relative humidity.
 *
 * @param[in]    N/A.
 *
 * @param[out]   myBatteryStatus:   The current battery status
 *
 *
 * @return       Status of SI7021_GetBatteryStatus.
 *
 *
 * @author      Manuel Caballero
 * @date        5/February/2018
 * @version     5/February/2018   The ORIGIN
 * @pre         N/A.
 * @warning     N/A.
 */
SI7021::SI7021_status_t  SI7021::SI7021_GetBatteryStatus ( SI7021_vector_data_t* myBatteryStatus )
{
    char      cmd      =    SI7021_READ_RH_T_USER_REGISTER_1;
    uint32_t  aux        =    0;


    aux = _i2c.write ( _SI7021_Addr, &cmd, 1, true );
    aux = _i2c.read  ( _SI7021_Addr, &cmd, 1 );


    myBatteryStatus->BatteryStatus   =   ( SI7021_vdds_status_t )( cmd & SI7021_VDDS_STATUS_MASK );





    if ( aux == I2C_SUCCESS )
        return   SI7021_SUCCESS;
    else
        return   SI7021_FAILURE;
}
