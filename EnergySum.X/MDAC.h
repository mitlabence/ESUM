#ifndef MDAC_HEADER
#define	MDAC_HEADER

#include <xc.h> // include processor files - each processor file is guarded.  
#include "SPI.h"

#define MDAC_1 0 //memory[MDAC_1] is memory register of MDAC 1

#define daisy_chain_length 4 //number of MDAC chips. Should be 4 for ESUM level 2, 12 for ESUM level 1.
#define MDAC_ControlWord 0b00001101 //for initializing the MDAC, see MDAC 21/28
#define MDAC_WriteA 0b00000001      //write command bits for AD5429 channel A, see 21/28
#define MDAC_WriteB 0b00000100      //write                         channel B
#define MDAC_ReadA  0b00000010      //read                          channel A
#define MDAC_ReadB  0b00000101      //read                          channel B
#define MDAC_NOP    0b00000000      //NOP

#define MDAC_disable_command 0b11101000     //disables SDO output driver for MDAC when sent after control bits 1101, see 20-21/28
#define MDAC_enable_command  0b00101000     //enables SDO for MDAC

#define MDAC_bytes_number   2       //MDAC commands consist of this many bytes; needed by SPI library (how many 8-bit blocks make up 1 command)

unsigned char MDAC_IO_Single(unsigned char control_bits, unsigned char data_bits);
void MDAC_CommandToArray(unsigned char control_bits, unsigned char data_bits, unsigned char *mdac_array);
unsigned char MDAC_GetNumber(unsigned char index);
unsigned char MDAC_GetChannel_AB(unsigned char index);
void MDAC_LoadUpdateSingle(unsigned char index, unsigned char data);
unsigned char MDAC_ReadSingle(unsigned char index);
void MDAC_Init(unsigned char SDO_control, unsigned char clear_to_midscale, unsigned char SCLK);
void MDAC_LoadUpdate(unsigned char first_index); //Use this to write to all 24 MDACs the corresponding memory[first_index + {0-23} ] value
void MDAC_Read(void);       //Use this to read the 24 MDAC values and write into memory[]
void MDAC_SendToAll(unsigned char control_bits, unsigned char data_byte);
void MDAC_SendToLast(unsigned char control_bits, unsigned char data_byte);

#endif	/* XC_HEADER_TEMPLATE_H */

