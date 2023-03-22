/**
 * @brief       PCF8574.h
 * @details     Remote 8-bit I/O expander for I2C-bus with interrupt.
 *              Functions file.
 *
 *
 * @return      NA
 *
 * @author      Manuel Caballero
 * @date        11/October/2017
 * @version     11/October/2017    The ORIGIN
 * @pre         NaN.
 * @warning     NaN
 * @pre         This code belongs to AqueronteBlog ( http://unbarquero.blogspot.com ).
 */

#include "PCF8574.h"


PCF8574::PCF8574 ( PinName sda, PinName scl, uint32_t addr, uint32_t freq )
    : i2c          ( sda, scl )
    , PCF8574_Addr ( addr )
{
    i2c.frequency( freq );
}


PCF8574::~PCF8574()
{
}



/**
 * @brief       PCF8574_SetPins ( PCF8574_vector_data_t )
 *
 * @details     It configures/sets the pins of the device.
 *
 * @param[in]    myConfDATA:        Data to set up the device.
 *
 * @param[out]   NaN.
 *
 *
 * @return       Status of PCF8574_SetPins.
 *
 *
 * @author      Manuel Caballero
 * @date        11/October/2017
 * @version     11/October/2017   The ORIGIN
 * @pre         NaN
 * @warning     NaN.
 */
PCF8574::PCF8574_status_t  PCF8574::PCF8574_SetPins ( PCF8574_vector_data_t  myConfDATA )
{
    uint32_t    aux                 =    0;


    aux = i2c.write ( PCF8574_Addr, &myConfDATA.data, 1 );



    if ( aux == I2C_SUCCESS )
       return   PCF8574_SUCCESS;
    else
       return   PCF8574_FAILURE;
}



/**
 * @brief       PCF8574_ReadPins ( PCF8574_vector_data_t*  )
 *
 * @details     It gets the data from the device ( port status ).
 *
 * @param[in]    NaN.
 *
 * @param[out]   myReadDATA:        ADC result into the chosen channel.
 *
 *
 * @return       Status of PCF8574_ReadPins.
 *
 *
 * @author      Manuel Caballero
 * @date        11/October/2017
 * @version     11/October/2017   The ORIGIN
 * @pre         NaN
 * @warning     NaN.
 */
PCF8574::PCF8574_status_t  PCF8574::PCF8574_ReadPins ( PCF8574_vector_data_t* myReadDATA )
{
    uint32_t    aux                 =    0;


    aux = i2c.read ( PCF8574_Addr, &myReadDATA->data, 1 );



    if ( aux == I2C_SUCCESS )
       return   PCF8574_SUCCESS;
    else
       return   PCF8574_FAILURE;
}
