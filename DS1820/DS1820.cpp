#include "DS1820.h"
#include "mbed_thread.h"

#ifdef TARGET_STM
//STM targets use opendrain mode since their switching between input and output is slow
    #define ONEWIRE_INPUT(pin)  pin->write(1)
    #define ONEWIRE_OUTPUT(pin) 
    #define ONEWIRE_INIT(pin)   pin->output(); pin->mode(OpenDrain)
#else
    #define ONEWIRE_INPUT(pin)  pin->input()
    #define ONEWIRE_OUTPUT(pin) pin->output()
    #define ONEWIRE_INIT(pin)
#endif

#ifdef TARGET_NORDIC
//NORDIC targets (NRF) use software delays since their ticker uses a 32kHz clock
    static uint32_t loops_per_us = 0;
    
    #define INIT_DELAY      init_soft_delay()
    #define ONEWIRE_DELAY_US(value) for(int cnt = 0; cnt < (value * loops_per_us) >> 5; cnt++) {__NOP(); __NOP(); __NOP();}
    
void init_soft_delay( void ) {
    if (loops_per_us == 0) {
        loops_per_us = 1;
        Timer timey; 
        timey.start();
        ONEWIRE_DELAY_US(320000);                     
        timey.stop();
        loops_per_us = (320000 + timey.read_us() / 2) / timey.read_us();  
    }
}
#else
    #define INIT_DELAY
    #define ONEWIRE_DELAY_US(value) wait_us(value)
#endif

LinkedList<node> DS1820::probes;
 
 
DS1820::DS1820 (PinName data_pin, PinName power_pin, bool power_polarity) : _datapin(data_pin), _parasitepin(power_pin) {
    int byte_counter;
    _power_polarity = power_polarity;

    _power_mosfet = power_pin != NC;
    
    for(byte_counter=0;byte_counter<9;byte_counter++)
        RAM[byte_counter] = 0x00;
    
    ONEWIRE_INIT((&_datapin));
    INIT_DELAY;
    
    if (!unassignedProbe(&_datapin, _ROM))
        error("No unassigned DS1820 found!\n");
    else {
        _datapin.input();
        probes.append(this);
        _parasite_power = !read_power_supply();
    }
}

DS1820::~DS1820 (void) {
    node *tmp;
    for(int i=1; i<=probes.length(); i++)
    {
        tmp = probes.pop(i);
        if (tmp->data == this)
            probes.remove(i);
    }
}

 
bool DS1820::onewire_reset(DigitalInOut *pin) {
// This will return false if no devices are present on the data bus
    bool presence=false;
    ONEWIRE_OUTPUT(pin);
    pin->write(0);          // bring low for 500 us
    ONEWIRE_DELAY_US(500);
    ONEWIRE_INPUT(pin);       // let the data line float high
    ONEWIRE_DELAY_US(90);            // wait 90us
    if (pin->read()==0) // see if any devices are pulling the data line low
        presence=true;
    ONEWIRE_DELAY_US(410);
    return presence;
}
 
void DS1820::onewire_bit_out (DigitalInOut *pin, bool bit_data) {
    ONEWIRE_OUTPUT(pin);
    pin->write(0);
    ONEWIRE_DELAY_US(3);                 // DXP modified from 5
    if (bit_data) {
        pin->write(1); // bring data line high
        ONEWIRE_DELAY_US(55);
    } else {
        ONEWIRE_DELAY_US(55);            // keep data line low
        pin->write(1);
        ONEWIRE_DELAY_US(10);            // DXP added to allow bus to float high before next bit_out
    }
}
 
void DS1820::onewire_byte_out(char data) { // output data character (least sig bit first).
    int n;
    for (n=0; n<8; n++) {
        onewire_bit_out(&this->_datapin, data & 0x01);
        data = data >> 1; // now the next bit is in the least sig bit position.
    }
}
 
bool DS1820::onewire_bit_in(DigitalInOut *pin) {
    bool answer;
    ONEWIRE_OUTPUT(pin);
    pin->write(0);
    ONEWIRE_DELAY_US(3);                 // DXP modofied from 5
    ONEWIRE_INPUT(pin);
    ONEWIRE_DELAY_US(10);                // DXP modified from 5
    answer = pin->read();
    ONEWIRE_DELAY_US(45);                // DXP modified from 50
    return answer;
}
 
char DS1820::onewire_byte_in() { // read byte, least sig byte first
    char answer = 0x00;
    int i;
    for (i=0; i<8; i++) {
        answer = answer >> 1; // shift over to make room for the next bit
        if (onewire_bit_in(&this->_datapin))
            answer = answer | 0x80; // if the data port is high, make this bit a 1
    }
    return answer;
}

bool DS1820::unassignedProbe(PinName pin) {
    DigitalInOut _pin(pin);
    ONEWIRE_INIT((&_pin));
    INIT_DELAY;
    char ROM_address[8];
    return search_ROM_routine(&_pin, 0xF0, ROM_address);
}
 
bool DS1820::unassignedProbe(DigitalInOut *pin, char *ROM_address) {
    return search_ROM_routine(pin, 0xF0, ROM_address);
}
 
bool DS1820::search_ROM_routine(DigitalInOut *pin, char command, char *ROM_address) {
    bool DS1820_done_flag = false;
    int DS1820_last_descrepancy = 0;
    char DS1820_search_ROM[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    
    int descrepancy_marker, ROM_bit_index;
    bool return_value, Bit_A, Bit_B;
    char byte_counter, bit_mask;
 
    return_value=false;
    while (!DS1820_done_flag) {
        if (!onewire_reset(pin)) {
            return false;
        } else {
            ROM_bit_index=1;
            descrepancy_marker=0;
            char command_shift = command;
            for (int n=0; n<8; n++) {           // Search ROM command or Search Alarm command
                onewire_bit_out(pin, command_shift & 0x01);
                command_shift = command_shift >> 1; // now the next bit is in the least sig bit position.
            } 
            byte_counter = 0;
            bit_mask = 0x01;
            while (ROM_bit_index<=64) {
                Bit_A = onewire_bit_in(pin);
                Bit_B = onewire_bit_in(pin);
                if (Bit_A & Bit_B) {
                    descrepancy_marker = 0; // data read error, this should never happen
                    ROM_bit_index = 0xFF;
                } else {
                    if (Bit_A | Bit_B) {
                        // Set ROM bit to Bit_A
                        if (Bit_A) {
                            DS1820_search_ROM[byte_counter] = DS1820_search_ROM[byte_counter] | bit_mask; // Set ROM bit to one
                        } else {
                            DS1820_search_ROM[byte_counter] = DS1820_search_ROM[byte_counter] & ~bit_mask; // Set ROM bit to zero
                        }
                    } else {
                        // both bits A and B are low, so there are two or more devices present
                        if ( ROM_bit_index == DS1820_last_descrepancy ) {
                            DS1820_search_ROM[byte_counter] = DS1820_search_ROM[byte_counter] | bit_mask; // Set ROM bit to one
                        } else {
                            if ( ROM_bit_index > DS1820_last_descrepancy ) {
                                DS1820_search_ROM[byte_counter] = DS1820_search_ROM[byte_counter] & ~bit_mask; // Set ROM bit to zero
                                descrepancy_marker = ROM_bit_index;
                            } else {
                                if (( DS1820_search_ROM[byte_counter] & bit_mask) == 0x00 )
                                    descrepancy_marker = ROM_bit_index;
                            }
                        }
                    }
                    onewire_bit_out (pin, DS1820_search_ROM[byte_counter] & bit_mask);
                    ROM_bit_index++;
                    if (bit_mask & 0x80) {
                        byte_counter++;
                        bit_mask = 0x01;
                    } else {
                        bit_mask = bit_mask << 1;
                    }
                }
            }
            DS1820_last_descrepancy = descrepancy_marker;
            if (ROM_bit_index != 0xFF) {
                int i = 1;
                node *list_container;
                while(1) {
                    list_container = probes.pop(i);
                    if (list_container == NULL) {                             //End of list, or empty list
                        if (ROM_checksum_error(DS1820_search_ROM)) {          // Check the CRC
                            return false;
                        }
                        for(byte_counter=0;byte_counter<8;byte_counter++)
                            ROM_address[byte_counter] = DS1820_search_ROM[byte_counter];
                        return true;
                    } else {                    //Otherwise, check if ROM is already known
                        bool equal = true;
                        DS1820 *pointer = (DS1820*) list_container->data;
                        char *ROM_compare = pointer->_ROM;
                        
                        for(byte_counter=0;byte_counter<8;byte_counter++) {
                            if ( ROM_compare[byte_counter] != DS1820_search_ROM[byte_counter])
                                equal = false;
                        }
                        if (equal)
                            break;
                        else
                            i++;
                    }
                }                        
            }
        }
        if (DS1820_last_descrepancy == 0)
            DS1820_done_flag = true;
    }
    return return_value;
}
 
void DS1820::match_ROM() {
// Used to select a specific device
    int i;
    onewire_reset(&this->_datapin);
    onewire_byte_out( 0x55);  //Match ROM command
    for (i=0;i<8;i++) {
        onewire_byte_out(_ROM[i]);
    }
}
 
void DS1820::skip_ROM() {
    onewire_reset(&this->_datapin);
    onewire_byte_out(0xCC);   // Skip ROM command
}
 
bool DS1820::ROM_checksum_error(char *_ROM_address) {
    char _CRC=0x00;
    int i;
    for(i=0;i<7;i++) // Only going to shift the lower 7 bytes
        _CRC = CRC_byte(_CRC, _ROM_address[i]);
    // After 7 bytes CRC should equal the 8th byte (ROM CRC)
    return (_CRC!=_ROM_address[7]); // will return true if there is a CRC checksum mis-match         
}
 
bool DS1820::RAM_checksum_error() {
    char _CRC=0x00;
    int i;
    for(i=0;i<8;i++) // Only going to shift the lower 8 bytes
        _CRC = CRC_byte(_CRC, RAM[i]);
    // After 8 bytes CRC should equal the 9th byte (RAM CRC)
    return (_CRC!=RAM[8]); // will return true if there is a CRC checksum mis-match        
}
 
char DS1820::CRC_byte (char _CRC, char byte ) {
    int j;
    for(j=0;j<8;j++) {
        if ((byte & 0x01 ) ^ (_CRC & 0x01)) {
            // DATA ^ LSB CRC = 1
            _CRC = _CRC>>1;
            // Set the MSB to 1
            _CRC = _CRC | 0x80;
            // Check bit 3
            if (_CRC & 0x04) {
                _CRC = _CRC & 0xFB; // Bit 3 is set, so clear it
            } else {
                _CRC = _CRC | 0x04; // Bit 3 is clear, so set it
            }
            // Check bit 4
            if (_CRC & 0x08) {
                _CRC = _CRC & 0xF7; // Bit 4 is set, so clear it
            } else {
                _CRC = _CRC | 0x08; // Bit 4 is clear, so set it
            }
        } else {
            // DATA ^ LSB CRC = 0
            _CRC = _CRC>>1;
            // clear MSB
            _CRC = _CRC & 0x7F;
            // No need to check bits, with DATA ^ LSB CRC = 0, they will remain unchanged
        }
        byte = byte>>1;
    }
return _CRC;
}
 
int DS1820::convertTemperature(bool wait, devices device) {
    // Convert temperature into scratchpad RAM for all devices at once
    int delay_time = 750; // Default delay time
    char resolution;
    if (device==all_devices)
        skip_ROM();          // Skip ROM command, will convert for ALL devices
    else {
        match_ROM();
        if ((FAMILY_CODE == FAMILY_CODE_DS18B20 ) || (FAMILY_CODE == FAMILY_CODE_DS1822 )) {
            resolution = RAM[4] & 0x60;
            if (resolution == 0x00) // 9 bits
                delay_time = 94;
            if (resolution == 0x20) // 10 bits
                delay_time = 188;
            if (resolution == 0x40) // 11 bits. Note 12bits uses the 750ms default
                delay_time = 375;
        }
    }
    
    onewire_byte_out( 0x44);  // perform temperature conversion
    if (_parasite_power) {
        if (_power_mosfet) {
            _parasitepin = _power_polarity;     // Parasite power strong pullup
            thread_sleep_for(delay_time);
            _parasitepin = !_power_polarity;
            delay_time = 0;
        } else {
            _datapin.output();
            _datapin.write(1);
            thread_sleep_for(delay_time);
            _datapin.input();
        }
    } else {
        if (wait) {
            thread_sleep_for(delay_time);
            delay_time = 0;
        }
    }
    return delay_time;
}
 
void DS1820::read_RAM() {
    // This will copy the DS1820's 9 bytes of RAM data
    // into the objects RAM array. Functions that use
    // RAM values will automaticly call this procedure.
    int i;
    match_ROM();             // Select this device
    onewire_byte_out( 0xBE);   //Read Scratchpad command
    for(i=0;i<9;i++) {
        RAM[i] = onewire_byte_in();
    }
//    if (!RAM_checksum_error())
//       crcerr = 1;
}

bool DS1820::setResolution(unsigned int resolution) {
    bool answer = false;
    resolution = resolution - 9;
    if (resolution < 4) {
        resolution = resolution<<5; // align the bits
        RAM[4] = (RAM[4] & 0x60) | resolution; // mask out old data, insert new
        write_scratchpad ((RAM[2]<<8) + RAM[3]);
//        store_scratchpad (DS1820::this_device); // Need to test if this is required
        answer = true;
    }
    return answer;
}
 
void DS1820::write_scratchpad(int data) {
    RAM[3] = data;
    RAM[2] = data>>8;
    match_ROM();
    onewire_byte_out(0x4E);   // Copy scratchpad into DS1820 ram memory
    onewire_byte_out(RAM[2]); // T(H)
    onewire_byte_out(RAM[3]); // T(L)
    if ((FAMILY_CODE == FAMILY_CODE_DS18B20 ) || (FAMILY_CODE == FAMILY_CODE_DS1822 )) {
        onewire_byte_out(RAM[4]); // Configuration register
    }
}
 
float DS1820::temperature(char scale) {
// The data specs state that count_per_degree should be 0x10 (16), I found my devices
// to have a count_per_degree of 0x4B (75). With the standard resolution of 1/2 deg C
// this allowed an expanded resolution of 1/150th of a deg C. I wouldn't rely on this
// being super acurate, but it does allow for a smooth display in the 1/10ths of a
// deg C or F scales.
    float answer, remaining_count, count_per_degree;
    int reading;
    read_RAM();
    if (RAM_checksum_error())
        // Indicate we got a CRC error
        answer = invalid_conversion;
    else {
        reading = (RAM[1] << 8) + RAM[0];
        if (reading & 0x8000) { // negative degrees C
            reading = 0-((reading ^ 0xffff) + 1); // 2's comp then convert to signed int
        }
        answer = reading +0.0; // convert to floating point
        if ((FAMILY_CODE == FAMILY_CODE_DS18B20 ) || (FAMILY_CODE == FAMILY_CODE_DS1822 )) {
            answer = answer / 16.0f;
        }
        else {
            remaining_count = RAM[6];
            count_per_degree = RAM[7];
            answer = floor(answer/2.0f) - 0.25f + (count_per_degree - remaining_count) / count_per_degree;
        }
        if (scale=='F' or scale=='f')
            // Convert to deg F
            answer = answer * 9.0f / 5.0f + 32.0f;
    }
    return answer;
}
 
bool DS1820::read_power_supply(devices device) {
// This will return true if the device (or all devices) are Vcc powered
// This will return false if the device (or ANY device) is parasite powered
    if (device==all_devices)
        skip_ROM();          // Skip ROM command, will poll for any device using parasite power
    else
        match_ROM();
    onewire_byte_out(0xB4);   // Read power supply command
    return onewire_bit_in(&this->_datapin);
}


