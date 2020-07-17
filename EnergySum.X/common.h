#ifndef COMMON_HEADER
#define	COMMON_HEADER

#include <xc.h> // include processor files - each processor file is guarded. 
#include <stdint.h>

#define reset_flag 0xbb
#define bootloader_flag 0x8000 //User ID register
#define bootloader_flag_MSB 0x80
#define bootloader_flag_LSB 0x00

#define baseline_ADC_value  0x024D //=589, ADC measured value when baseline is 0 TODO: this changes with temperature etc.?
#define VOLATILE_DAC0_ADDRESS 0x00 

void ODAC_SetValue(uint16_t value);
uint16_t SetBaseline(void);
void nvm_unlock(void);
void flash_memory_erase (unsigned int address, unsigned char erase_config_registers);
void flash_memory_set_bootloader_flag(void);

#endif

