#define I2C_INITIAL_STATE           0x00
#define I2C_SLAVE_ADDRESS_RECEIVED  0x01
#define I2C_CONTROL_WORD_RECEIVED   0x02
#define COMMAND_SET_FLASH_ADDRESS   0x01
#define COMMAND_WRITE_TO_BUFFER     0x02
#define COMMAND_READ_FROM_FLASH     0x03
#define COMMAND_ERASE_FLASH_ROW     0x04
#define COMMAND_WRITE_TO_FLASH      0x05
#define COMMAND_START_FIRMWARE      0x06
#define COMMAND_CONTINUE_READ_FROM_FLASH 0x07   //the Arduino used for hex file uploading can not handle more than 32 bytes, which is half the buffer size -> need command to read out second half
#define COMMAND_RESET               0x08

#define CRC_WIDTH 8
#define CRC_TOPBIT (1 << (CRC_WIDTH - 1))
#define CRC_POLYNOMIAL 0x1D
#define CRC_INITIAL_REMAINDER 0xFF
#define CRC_FINAL_XOR_VALUE 0xFF

#define _XTAL_FREQ 32000000

#include <xc.h> // include processor files - each processor file is guarded.  

void I2C_SlaveInit(unsigned char slave_address);
void I2C_SendBack(unsigned char data);
void I2C_Communicate(void);
unsigned char I2C_CalculateCRC(void);