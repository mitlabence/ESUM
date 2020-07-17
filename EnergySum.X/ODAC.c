/*
 * Library for the Offset DAC controlling functions
 * Chip: MCP48FVB21 -E/UN
 * Data sheet p. 4/84:
 * # of channels:           1
 * Resolution:              12 bits
 * DAC output POR/BOR set.  7FFh
 * # VRef inputs            1
 * Internal band gap?       Yes
 * # of /LAT inputs         1
 * Specified VDD op. range  1.8V to 5.5V
 *           
 * As of 29.03.2019: VOUT pin ~509 mV: for about 0x360
 * 
 * Device Block Diagram:             2/84
 * Timing diagrams:                 20/84
 * Overview of memory registers:    32/84
 * Volatile DAC0 DAC1 Registers:    33/84
 * Gain control register:           36/84
 * SPI:                             49/84
 * Structure of a command:          52/84
 */

#define VOL_DAC0_ADDRESS    0x00    // 00h: Volatile DAC0 Register, p. 32/84
#define VOL_DAC1_ADDRESS    0x01    // 01h: Volatile DAC1 Register (only for 2-channel chip, that is, MCP...x2)
#define GAIN_STATUS_ADDRESS 0x0a    // 0Ah: Gain and Status Register
#define VREF_ADDRESS        0x08    // 08h: VREF control register address, p. 34/84
#define ODAC_WRITE_COMMAND  0b00    // manual 52/84
#define ODAC_READ_COMMAND   0b11
#define ODAC_BAND_GAP_MODE  0b01
#define ODAC_bytes_number   3

#include <stdbool.h>
#include <stdint.h>
#include <xc.h>
#include "SPI.h"
#include "ODAC.h"



/*
 * Options, see datasheet p. 34/84:
 * mode
 * 0    Device VDD (unbuffered)
 * 1    Internal Band Gap (1.22 V typical)    
 * 2    External VREF pin (unbuffered)
 * 3    External VREF pin (buffered)
 * See p.39/84 and 34/84
 */
void ODAC_SelectReference(unsigned char mode){
    uint16_t data_bits = 0;
    data_bits |= mode;
    Set_CS(0);  //Activate DAC CS
    ODAC_IO(VREF_ADDRESS, 1, data_bits);
    Set_CS(2);
}


/* Fills the passed odac_array with 8-bit blocks ready to use for the SPI library.
 * Command structure: p. 52/84
 * the array has the structure:
 * 
 * [A4 A3 A2 A1 A0 C1 C0 CMDERR,  
 *  D15 D14 ...              D8,
 *  D7  D6  ...              D0] 
 * 
 * memory_address:
 *  0 0 0 A4 A3 A2 A1 A0
 * 
 * is_read_mode:
 *      0       Write to memory location (20 MHz max)
 *      1       Read from memory location (10 MHz max)
 * 
 * data_word:
 *      0 0 0 0 D11 D10 ... D0
 */
void ODAC_CommandToArray(unsigned char memory_address, unsigned char is_write_mode, uint16_t data_word, unsigned char *odac_array){
    odac_array[0] = 0;  //A4 A3 A2 A1 A0 C1 C0 CMDERR
    odac_array[1] = 0;  //D15 D14 ... D8
    odac_array[2] = 0;  //D7 D6 ... D0
    
    odac_array[0] |= memory_address;
    odac_array[0] <<= 2;
    if(is_write_mode == 0){
        odac_array[0] |= ODAC_READ_COMMAND;
    }
    else{
        odac_array[0] |= ODAC_WRITE_COMMAND;
    }
    odac_array[0] <<= 1;
    
    uint16_t helpful_var;
    helpful_var = data_word;
    odac_array[2] |= helpful_var;
    helpful_var >>= 8;
    odac_array[1] |= helpful_var;
}

/*
 * Does a single I/O process with the Offset DAC, returns the two bytes of data.
 * 
 * Command structure (52/84):
 * A4 A3 A2 A1 A0 C1 C0 CMDERR D15 D14 ... D0
 * 
 * memory_address:  values at 32/84
 * is_write_mode:   1 for writing to memory address, 0 for reading
 * data_word:       12-bit value (0x000 to 0xfff) in form of 16-bit number
 */
uint16_t ODAC_IO(unsigned char memory_address, unsigned char is_write_mode, uint16_t data_word){
    uint16_t answer;
    unsigned char ODAC_array[ODAC_bytes_number];
    answer = 0;
    ODAC_CommandToArray(memory_address, is_write_mode, data_word, ODAC_array);
    SPI_IO(ODAC_array, ODAC_bytes_number);
    //get 16-bit answer from 24-bit MISO signal
    answer |= ODAC_array[1];
    answer <<= 8;
    answer |= ODAC_array[2];
    return answer;
}

/*
 * Set ODAC gain to 1x (2x if internal reference used) or 2x (4x if internal reference used)
 * gain_setting=0 sets lower gain setting
 * gain_setting=1 sets higher gain setting
 * See: 36/84
 * 
 * Memory address: 0Ah = 0x0A
 * Need to change G0 bit = bit 8 (0-15)
 * 
 * TODO: not tested yet!
 */
void ODAC_SetGain(unsigned char gain_setting){
    uint16_t gain_register  = gain_setting;
    gain_register <<= 8; 
    ODAC_IO(0x0a, 1, gain_register);
}

uint16_t ODAC_ReadValue(void){
    uint16_t read_value;
    Set_CS(0);
    read_value = ODAC_IO(VOL_DAC0_ADDRESS, 0, 0);
    Set_CS(2);
    return read_value;
}