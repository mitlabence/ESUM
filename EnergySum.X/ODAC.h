#ifndef ODAC_HEADER
#define	ODAC_HEADER

#include <stdbool.h>
#include <stdint.h>
#include <xc.h>
#include "SPI.h"



#define VOL_DAC0_ADDRESS    0x00    // 00h: Volatile DAC0 Register, p. 32/84
#define VOL_DAC1_ADDRESS    0x01    // 01h: Volatile DAC1 Register (only for 2-channel chip, that is, MCP...x2)
#define GAIN_STATUS_ADDRESS 0x0a    // 0Ah: Gain and Status Register
#define VREF_ADDRESS        0x08    // 08h: VREF control register address, p. 34/84
#define ODAC_WRITE_COMMAND  0b00    // manual 52/84
#define ODAC_READ_COMMAND   0b11
#define ODAC_BAND_GAP_MODE  0b01
#define ODAC_bytes_number   3

void ODAC_SelectReference(unsigned char mode);
void ODAC_CommandToArray(unsigned char memory_address, unsigned char is_write_mode, uint16_t data_word, unsigned char *odac_array);
uint16_t ODAC_IO(unsigned char memory_address, unsigned char is_write_mode, uint16_t data_word);
void ODAC_SetGain(unsigned char gain_setting);
uint16_t ODAC_ReadValue(void);

#endif

