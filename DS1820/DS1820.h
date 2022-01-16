/* mbed DS1820 Library, for the Dallas (Maxim) 1-Wire Digital Thermometer
 * Copyright (c) 2010, Michael Hagberg Michael@RedBoxCode.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MBED_DS1820_H
#define MBED_DS1820_H

#include "mbed.h"
#include "LinkedList.h"

#define FAMILY_CODE _ROM[0]
#define FAMILY_CODE_DS1820 0x10
#define FAMILY_CODE_DS18B20 0x28
#define FAMILY_CODE_DS1822  0x22

/** DS1820 Dallas 1-Wire Temperature Probe
 *
 * Example:
 * @code
 * #include "mbed.h"
 * #include "DS1820.h"
 *
 * DS1820 probe(DATA_PIN);
 *  
 * int main() {
 *     while(1) {
 *         probe.convertTemperature(true, DS1820::all_devices);         //Start temperature conversion, wait until ready
 *         printf("It is %3.1foC\r\n", probe.temperature());
 *         wait(1);
 *     }
 * }
 * @endcode
 */
class DS1820 {
public:
    enum devices{
        this_device,     // command applies to only this device
        all_devices };   // command applies to all devices
    
    enum {
        invalid_conversion = -1000
    };

    /** Create a probe object connected to the specified pins
    *
    * The probe might either by regular powered or parasite powered. If it is parasite
    * powered and power_pin is set, that pin will be used to switch an external mosfet connecting
    * data to Vdd. If it is parasite powered and the pin is not set, the regular data pin
    * is used to supply extra power when required. This will be sufficient as long as the 
    * number of probes is limitted.
     *
     * @param data_pin DigitalInOut pin for the data bus
     * @param power_pin DigitalOut (optional) pin to control the power MOSFET
     * @param power_polarity bool (optional) which sets active state (0 for active low (default), 1 for active high)
     */
    DS1820(PinName data_pin, PinName power_pin = NC, bool power_polarity = 0); // Constructor with parasite power pin
    ~DS1820();

    /** Function to see if there are DS1820 devices left on a pin which do not have a corresponding DS1820 object
    *
    * @return - true if there are one or more unassigned devices, otherwise false
      */
    static bool unassignedProbe(PinName pin);

    /** This routine will initiate the temperature conversion within
      * one or all DS1820 probes. 
      *
      * @param wait if true or parisitic power is used, waits up to 750 ms for 
      * conversion otherwise returns immediatly.
      * @param device allows the function to apply to a specific device or
      * to all devices on the 1-Wire bus.
      * @returns milliseconds untill conversion will complete.
      */
    int convertTemperature(bool wait, devices device=all_devices);

    /** This function will return the probe temperature. Approximately 10ms per
      * probe to read its RAM, do CRC check and convert temperature on the LPC1768.
      *
      * @param scale, may be either 'c' or 'f'
      * @returns temperature for that scale, or DS1820::invalid_conversion (-1000) if CRC error detected.
      */
    float temperature(char scale='c');

    /** This function sets the temperature resolution for the DS18B20
      * in the configuration register.
      *
      * @param a number between 9 and 12 to specify resolution
      * @returns true if successful
      */ 
    bool setResolution(unsigned int resolution);       

private:
    bool _parasite_power;
    bool _power_mosfet;
    bool _power_polarity;
    
    static char CRC_byte(char _CRC, char byte );
    static bool onewire_reset(DigitalInOut *pin);
    void match_ROM();
    void skip_ROM();
    static bool search_ROM_routine(DigitalInOut *pin, char command, char *ROM_address);
    static void onewire_bit_out (DigitalInOut *pin, bool bit_data);
    void onewire_byte_out(char data);
    static bool onewire_bit_in(DigitalInOut *pin);
    char onewire_byte_in();
    static bool ROM_checksum_error(char *_ROM_address);
    bool RAM_checksum_error();
    void read_RAM();
    static bool unassignedProbe(DigitalInOut *pin, char *ROM_address);
    void write_scratchpad(int data);
    bool read_power_supply(devices device=this_device);

    DigitalInOut _datapin;
    DigitalOut _parasitepin;
    
    char _ROM[8];
    char RAM[9];
    
    static LinkedList<node> probes;
};


#endif