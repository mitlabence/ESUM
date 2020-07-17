#ifndef I2C_HEADER
#define	I2C_HEADER

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdbool.h>

void I2C_MasterInit(void);
void I2C_Start(void);
void I2C_Restart(void);
void I2C_Stop(void);
void I2C_WriteByte(unsigned char control_byte, unsigned char data);
void I2C_ReadAddress(unsigned char control_byte_write, unsigned char control_byte_read, unsigned char memory_address, unsigned char* data, unsigned char data_length);
void ReadGlobalAddress(unsigned char* data, unsigned char data_length);
void I2C_SlaveInit(unsigned char slave_address);

#endif

