#include <xc.h>
#include "i2c.h"
#include "flash.h"
#include "typedef.h"

extern unsigned char i2c_status;
extern unsigned char i2c_control_word;
extern ADDRESS flash_address;
extern unsigned char flash_address_index; //0 refers to MSB, 1 to LSB
extern unsigned char flash_buffer[64];  //32 words in one row = 64 bytes
extern const unsigned char lookup_table[256];
extern unsigned char flash_buffer_index; //global index of flash_buffer
extern unsigned char reset_timer;

//TODO: Bootloader with i2c-address 0x2f does not work, CRC mismatch happens. Probably the lookup table needs to be rewritten for this feature (0x1f and 0x2f i2c addresses for the various stage modules) to work.

/*
 * Initializes I2C slave mode on MSSP2 register for PIC16F18345, with slave address:
 * 
 * x S6 S5 S4 S3 S2 S1 S0           (7-bit address)
 * 
 * 
 * Description:                     327/491
 * I2C slave address: SSPxADD,      364/491
 * PIE2                             104
 * INTCON (interrupt control)       101
 * 
 * SCL pin: RC3 (pin 7)
 * SDA pin: RC6 (pin 8)
 * 
 * NO interrupts are utilized, only the SSP2IF can be used in if() statements!
 */
void I2C_SlaveInit(unsigned char slave_address){    //slave_address of the form: x S6 S5 ... S0!
    //PIE2bits.SSP2IE = 1;            //Enable SSP2 interrupt (not needed actually, this setting is not responsible for the interrupt flag)       
    //Change RC3 and RC6 to digital pins, 157
    GIE = 0;
    ANSELCbits.ANSC3 = 0;
    ANSELCbits.ANSC6 = 0;
    

    
    // 325/491: 30.4.3: "When selecting any I2C mode, the SCL and SDA pins should be set by the user to inputs..."
    TRISCbits.TRISC3 = 1;           //Set RC4 (MISO) to input
    TRISCbits.TRISC6 = 1;           //Set RC4 (MISO) to input
     
    PMD4bits.MSSP2MD = 0;           //Enable MSSP2 module (default is also 0 though), 170
     
     
    //p. 359
    SSP2STATbits.SMP = 0;//should be 0
    SSP2STATbits.CKE = 0;

    //Configuring pins, see 361/491 (note 2)
    PPSLOCK = 0;                    //Enable writing (also 0 by default) 164
    
    //Set 7-bit address
    unsigned char address = slave_address;
    address <<= 1;              //364/491
    SSP2ADD = address;
    
    
    //p. 325 Note: SDA input: SSPDATPPS, SCL input: SSPCLKPPS, outputs: RxyPPS, see p. 162
    //TODO: some functions here (see note 1) automatically set TRIS (-> TRIS setup in beginning unnecessary)?
    RC6PPS = 0b11011;   //set RC6 to SDA2
    SSP2DATPPS = 0b10110;
           
    
    RC3PPS = 0b11010;   //set RC3 to SCK2
    SSP2CLKPPS = 0b10011;
    
    //p. 360
    SSP2CON1 = 0b00110110; //THIS CAUSES BUGS WHEN TWO INITIALIZATIONS HAPPEN AFTER ANOTHER!!
    
    //p. 362
    SSP2CON2 = 0b0; //same as default settings
    SSP2CON3 = 0b0;
    //p. 363, also 325 30.4.4 SDA hold time
    SSP2IF = 0; //TODO: if I2C has problems, this might be the reason!
    flash_buffer_index = 0; //initialize buffer index to 0
}

void I2C_SendBack(unsigned char data){//slave sends master a byte
    do{
        SSP2CON1bits.WCOL = 0;
        SSP2BUF = data;
    }
    while(SSP2CON1bits.WCOL);
    SSP2CON1bits.CKP = 1;
    return;
}
/*
 * mask = 00100101
 * SSP2STAT:
 *  bit 0: BF (buffer full status bit) -> 1 if received recently (only 0 if last command was a read)
 *  bit 2: R_nW bit
 *  bit 5: D_nA bit
 * 
 * MWA: 00000001
 * MWD: 00100001
 * MRA: 00000101
 * MRD: 00100100
 */

/*
 * Write to flash:              129
 * Block graphics of write:     130
 * Flowchart of write:          131
 * NVM registers:               137
 * IMPORTANT: need to erase flash address before writing to it!!!
 */

/*
 * Possible commands:
 * 1a. Set flash memory address: slave address W, write 0x01, write LSB, write MSB.
 * 1b. Get flash memory address: slave address W, write 0x01, stop, slave address R, read 2 bytes. Returns MSB, then LSB.
 * 2. Write to flash buffer (16 bytes): slave address W, write 0x02, write 16 bytes, stop.
 * 3. Read from flash memory: (set flash memory address to set first value to read back) slave address W, write 0x03, stop, slave address R, read 16 bytes.
 * 4. Erase row from flash memory: (set flash memory address to an initial index of a row!) slave address W, write 0x04, stop, slave address R, read 1 byte (0x00).
 * 5. Write from buffer to flash memory: (set flash memory address to initial index to write to!) slave address W, write 0x05, stop, slave address R, read 1 byte (0x00).
 * 6. Restart PIC and start program: slave address W, write 0x06, stop, slave address R, read 1 byte (0x00).
 * 7. Write buffer to config bits: slave address W, write 0x07, stop, slave address R, read 1 byte.
 */


void I2C_Communicate(void){
    if(SSP2IF){
        unsigned char temp; //used to empty SSP2BUF to this variable
        unsigned int temp_word = 0; //used to read words from flash memory
        reset_timer = 1; //request a reset of the timer
        if(SSP2STATbits.S){
            if(SSP2STATbits.R_nW){//Read command
                if(SSP2STATbits.D_nA){//Data
                   switch(i2c_control_word){
                       case COMMAND_SET_FLASH_ADDRESS://send LSB back
                           I2C_SendBack(flash_address.bytes.MSbyte);
                           break;
                       case COMMAND_READ_FROM_FLASH:
                           I2C_SendBack(flash_buffer[flash_buffer_index]);
                           flash_buffer_index++;
                           if(flash_buffer_index == 64){//guard against index out of range
                               flash_buffer_index--;
                           }
                           break;
                       case COMMAND_CONTINUE_READ_FROM_FLASH:
                           I2C_SendBack(flash_buffer[flash_buffer_index]);
                           flash_buffer_index++;
                           if(flash_buffer_index == 64){//guard against index out of range
                               flash_buffer_index--;
                   }
                           break;
               }
               }
               else{//Address
                   switch(i2c_control_word){
                       case COMMAND_SET_FLASH_ADDRESS://in Read mode, this means reading back flash memory current address
                           I2C_SendBack(flash_address.bytes.LSbyte); //send LSB; MSB will be sent back in Data mode
                           break;
                       case COMMAND_READ_FROM_FLASH: //fill flash buffer with flash data, send first byte
                           for(unsigned char i = 0; i < 64; ++i){
                               temp_word = flash_memory_read(flash_address.word.address, 0);
                               flash_buffer[i] = temp_word>>8; //MSB
                               i++;
                               flash_buffer[i] = temp_word & 0xff; //LSB
                               flash_address.word.address++;
                           }        
                           flash_buffer_index = 0;
                           I2C_SendBack(flash_buffer[flash_buffer_index]); //index is 0; read-back continues with next byte (which is Data type, not Address)
                           flash_buffer_index++;
                           break;
                       case COMMAND_CONTINUE_READ_FROM_FLASH: //do not change buffer, but read out second half of it
                           flash_buffer_index = 32;
                           I2C_SendBack(flash_buffer[flash_buffer_index]);
                           flash_buffer_index++;
                           break;
                       case COMMAND_ERASE_FLASH_ROW:
                           I2C_SendBack(0x00);
                           flash_memory_erase(flash_address.word.address, 0); //pass 0 to access program flash memory instead of user ID etc.
                           //flash_address.word.address += 32; //whole row has been erased
                           break;
                       case COMMAND_WRITE_TO_FLASH: //write from buffer to memory with initial word marked by flash_address.word.address
                           //I2C_SendBack(0x00);
                           I2C_SendBack(I2C_CalculateCRC());
                           flash_memory_write(flash_address.word.address, flash_buffer, 64);
                           flash_address.word.address += 32;
                           flash_buffer_index = 0; //ready to fill buffer again.
                           break;
                       case COMMAND_START_FIRMWARE://restart and leave bootloader on startup
                           I2C_SendBack(0x00);
                           flash_memory_set_bootloader_flag(0x01); //set the flag to 1 to jump to program after a reset.
                           __delay_ms(1);
                           asm("RESET");
                           break;
                       case COMMAND_RESET:
                           I2C_SendBack(0x00);
                           asm("RESET");
                           break;
                       default:
                           I2C_SendBack(0x01); //return 1 if invalid command received
                           break;
                   }
               }
            }
            else{//Write command
                temp = SSP2BUF;
                if(SSP2STATbits.D_nA){
                        if(i2c_status == I2C_SLAVE_ADDRESS_RECEIVED){ //previous byte was slave address; this one is command word
                            i2c_control_word = temp;
                            i2c_status = I2C_CONTROL_WORD_RECEIVED;
                            flash_address_index = 0;
                            //flash_buffer_index = 0; //Commands should always use whole flash buffer, so set index to 0
                        }
                        else if(i2c_status == I2C_CONTROL_WORD_RECEIVED){
                        switch(i2c_control_word){ 
                            case COMMAND_SET_FLASH_ADDRESS: //Sending word data in smbus in python causes LSByte to be sent first -> makes sense to take LSByte first here as well.
                                if(flash_address_index == 1){//MSB is being set
                                    flash_address.bytes.MSbyte = temp;
                                    flash_address_index = 0;
                                }
                                else{
                                    flash_address.bytes.LSbyte = temp;
                                    flash_address_index = 1;
                                }
                                break;
                            case COMMAND_WRITE_TO_BUFFER://write to flash buffer
                                if (flash_buffer_index == 64){//guard against index out of range error
                                    flash_buffer_index--;
                                }
                                flash_buffer[flash_buffer_index] = temp;
                                flash_buffer_index++;
                                break;
                        }
                    }
                }
                else{//Slave address arrived in write mode
                    i2c_status = I2C_SLAVE_ADDRESS_RECEIVED;
                }
            }
        }
        else if(SSP2STATbits.P){	//STOP state	
            asm("NOP"); //TODO: why is it here?
            i2c_status = I2C_INITIAL_STATE; 
        }	
        SSP2IF = 0;												
        SSP2CON1bits.CKP = 1;	//release clock TODO: is this needed here?
     }
}

unsigned char I2C_CalculateCRC(void){
    unsigned char data;
    unsigned char remainder = CRC_INITIAL_REMAINDER;
    for(unsigned char i = 0; i < 64; i++){//64 is the size of flash_buffer[]
        data = flash_buffer[i] ^ (remainder >> (CRC_WIDTH - 8));
        remainder = lookup_table[data] ^ (remainder << 8);
    }
    return (remainder ^ CRC_FINAL_XOR_VALUE);
}