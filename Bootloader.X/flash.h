#include <xc.h> // include processor files - each processor file is guarded.  

#define _XTAL_FREQ 32000000
#define bootloader_flag_row 0x1FE0 //row of 32 words inside PIC memory. Should look like 3FFF 3FFF ... 3FFF <flag>, i.e. 0x1FFF is flag word.
#define bootloader_flag_address 0x8000 //first User ID word is reserved for bootloader
#define bootloader_flag_MSB 0x80
#define bootloader_flag_LSB 0x00

void nvm_unlock(void);
unsigned int flash_memory_read(unsigned int address, unsigned char read_config_registers);
void flash_memory_write (unsigned int address, unsigned char *data, unsigned char n_of_bytes );
void flash_memory_erase (unsigned int address, unsigned char erase_config_registers);
void flash_memory_set_bootloader_flag(unsigned char value);