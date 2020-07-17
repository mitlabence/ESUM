#include <xc.h>
#include "flash.h"

/*
 * PIC16F1345 data sheet 128/491:
 * The flash memory has addresses 0x0000 - 0x17FF.
 */


/*
 * 126/491
 */
void nvm_unlock(void){
    NVMCON2 = 0x55;
    NVMCON2 = 0xaa;
    NVMCON1bits.WR = 1;
}

/*
 * 125/491
 * 
 * read_user_id: 0 if read flash memory, 1 if read user id, eeprom, configuration, device ID registers
 */
unsigned int flash_memory_read(unsigned int address, unsigned char read_config_registers){
    NVMCON1bits.NVMREGS = read_config_registers; //access flash memory location
    NVMADRL = ((address)&0xff); //set address
    NVMADRH = ((address)>>8);
    NVMCON1bits.RD = 1; //initiate read
    unsigned int ret_val = ((NVMDATH)<<8 | (NVMDATL));
    //data should be available for read-out the very next cycle
    return ret_val;
    }	

/*
 * Procedure:   129/491
 * NVMCON1:     138
 */
void flash_memory_write (unsigned int address, unsigned char *data, unsigned char n_of_bytes ){ //as hex files are little-endian (LSByte first, i.e. 0xabcd is stored as cd ab), makes sense to store it in buffer the same way, and this is assumed here.
    NVMCON1bits.WREN = 1; //allow writing
    NVMCON1bits.NVMREGS = 0;
    NVMCON1bits.LWLO = 1;
    unsigned int adr = address;
    NVMADRL = ((adr)&0xff); //set address
    NVMADRH = ((adr)>>8);
    for(unsigned char i = 0; i < n_of_bytes-2; i+=2){//repeat process until all but last write latch has been loaded (data sheet 129 step 8)
        NVMDATL = data[i]; //load data to write to current NVM address
        NVMDATH = data[i+1]; //0xabcd in little-endian is cd ab, for example, in buffer, so reverse them
        nvm_unlock();
        NVMADR++;
    }
    NVMCON1bits.LWLO = 0; //next write to latch will initiate write process to flash
    NVMDATL = data[n_of_bytes-2];
    NVMDATH = data[n_of_bytes-1];
    nvm_unlock(); //unlock sequence, this time also initiates write to flash
    NVMCON1bits.WREN = 0; //inhibit writing
}
  
/*
 * 127/491
 * 11.4.4
 */
void flash_memory_erase(unsigned int address, unsigned char erase_user_id){//erase_config_registers = 0 if want to erase program flash memory; 1 if EEPROM, USER ID registers
    NVMCON1bits.NVMREGS = erase_user_id; //1. clear bit to erase Flash memory
    NVMADRL = ((address)&0xff); //2. set address
    NVMADRH = ((address)>>8); //for 0x0a, for example, will take value of 0x8a because first bit is constant 1; see 46/491
    NVMCON1bits.FREE = 1; //3. set FREE and WREN bits
    NVMCON1bits.WREN = 1;
    nvm_unlock(); //4. unlock sequence
}	

/*
 * 0x8000 (first User ID) contains the BL flag byte.
 * Useful overall for writing to program memory: http://ww1.microchip.com/downloads/en/DeviceDoc/40001738D.pdf
 */
void flash_memory_set_bootloader_flag(unsigned char value){
    flash_memory_erase(bootloader_flag_address, 1); //pass 1 to access user ID register
    
    NVMCON1bits.WREN = 1; //allow writing
    NVMCON1bits.NVMREGS = 1; //access user ID
    NVMCON1bits.LWLO = 0; //next write to latch will initiate write process to flash
    NVMADRL = bootloader_flag_LSB; //set address
    NVMADRH = bootloader_flag_MSB;
    NVMDATL = value; //write 1 to bootloader flag word to leave bootloader on reset
    NVMDATH = 0;
    nvm_unlock(); //unlock sequence, this time also initiates write to flash
    NVMCON1bits.WREN = 0; //inhibit writing
}


