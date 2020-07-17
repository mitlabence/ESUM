#ifndef SPI_HEADER
#define	SPI_HEADER

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdint.h>

void SPI_Init(unsigned char clock_mode, unsigned char CKE);
void Set_CS(unsigned char setting);
unsigned char SPI_IO_Byte(unsigned char data);
void SPI_IO(unsigned char *data_array, unsigned char length);
void SPI_Process(void);

#endif

