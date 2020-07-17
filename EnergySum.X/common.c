#include <xc.h>
#include "common.h"
#include "SPI.h"
#include "MDAC.h"
#include "ODAC.h"
#include "ADC.h"
#include "I2C.h"



/*
 * Only DAC0 exists on currently used ...21 chip
 * VOUT pin ~509 mV: for about 0x360
 * 
 */
void ODAC_SetValue(uint16_t value){
    Set_CS(0);
    ODAC_IO(VOLATILE_DAC0_ADDRESS, 1, value);
    Set_CS(2);
}

//Sets baseline to 0 by using pre-defined ADC reference value baseline_ADC_value, and returns the corresponding offset DAC setting (12-bit number as uint16_t)
uint16_t SetBaseline(void){
    uint16_t current_bit_to_set = 0b100000000000;   //12-bit ODAC, start with highest bit, use interval halving method
    uint16_t baseline = 0;
    uint16_t ODAC_value = 0;
    
    SPI_Init(1,1);              //Initialize SPI for ODAC
    Set_CS(0);
    ODAC_SelectReference(1);    //Internal band gap
    Set_CS(2);
    ADC_Configure(0);           //0 is Baseline
    ODAC_SetValue(0);
    __delay_ms(200);           //can be lowered, only to follow process on oscilloscope in real-time
    for(current_bit_to_set = 0b100000000000; current_bit_to_set > 0; current_bit_to_set >>= 1){
        /*
         * Trying to get the ODAC_value (12 bit number) for which the ODAC offset makes the baseline 0.
         * The baseline is 0 if the measured baseline = baseline_ADC_value.
         * The baseline is too high (> 0 V) if the measured baseline < 0 (because it is measured by the ADC which gives 0 when too high voltage applied, and full 1-s if too low voltage)
         */
        ODAC_value |= current_bit_to_set;       //Assume the bit is needed; set it back to 0 later if not
        ODAC_SetValue(ODAC_value);
        __delay_ms(100);
        baseline = ADC_Measure();
        if(baseline < baseline_ADC_value){      //baseline too high, bit needs to be cleared
            ODAC_value ^= current_bit_to_set;
        }
    }
    Set_CS(0);
    ODAC_SetValue(ODAC_value);
    Set_CS(2);
    __delay_ms(100);
    return ODAC_value;
}


void nvm_unlock(void){
    NVMCON2 = 0x55;
    NVMCON2 = 0xaa;
    NVMCON1bits.WR = 1;
}

/*
 * 127/491
 * 11.4.4
 */
void flash_memory_erase(unsigned int address, unsigned char erase_config_registers){//erase_config_registers = 0 if want to erase program flash memory; 1 if EEPROM, USER ID registers
    NVMCON1bits.NVMREGS = erase_config_registers; //1. clear bit to erase Flash memory
    NVMADRL = ((address)&0xff); //2. set address
    NVMADRH = ((address)>>8); //for 0x0a, for example, will take value of 0x8a because first bit is constant 1; see 46/491
    NVMCON1bits.FREE = 1; //3. set FREE and WREN bits
    NVMCON1bits.WREN = 1;
    nvm_unlock(); //4. unlock sequence
}	


void flash_memory_set_bootloader_flag(void){
    flash_memory_erase(bootloader_flag, 1);//erase user ID register
    __delay_ms(20);
    
    NVMCON1bits.WREN = 1; //allow writing
    NVMCON1bits.NVMREGS = 0;
    NVMCON1bits.LWLO = 0; //next write to latch will initiate write process to flash
    NVMADRL = bootloader_flag_LSB; //set address
    NVMADRH = bootloader_flag_MSB;
    NVMDATL = 0; //write 1 to bootloader flag word to leave bootloader on reset, write 0 to stay in bootloader
    NVMDATH = 0;
    nvm_unlock(); //unlock sequence, this time also initiates write to flash
    NVMCON1bits.WREN = 0; //inhibit writing
}

/*
 * 0x8000 (first User ID) contains the BL flag byte.
 * Useful overall for writing to program memory: http://ww1.microchip.com/downloads/en/DeviceDoc/40001738D.pdf
 */
/*
void flash_memory_set_flag(void){
    NVMCON1bits.WREN = 1; //allow writing
    NVMCON1bits.NVMREGS = 1; //access user ID
    NVMCON1bits.LWLO = 0; //next write to latch will initiate write process to flash
    unsigned int adr = bootloader_flag_address;
    NVMADRL = ((adr)&0xff); //set address
    NVMADRH = ((adr)>>8);
    NVMDATL = 0; //write 1 to bootloader flag word to leave bootloader on reset
    NVMDATH = 0;
    nvm_unlock(); //unlock sequence, this time also initiates write to flash
    NVMCON1bits.WREN = 0; //inhibit writing
}
*/