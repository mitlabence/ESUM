#include <xc.h> // include processor files - each processor file is guarded.  
#include "typedef.h"

#define LED_1 LATA2
#define LED_2 LATA4
#define LED_3 LATA5

extern unsigned char i2c_status;
extern unsigned char i2c_control_word;
extern ADDRESS flash_address;
extern unsigned char flash_address_index;
extern unsigned char flash_buffer[64];
extern unsigned char flash_buffer_index;
extern unsigned char reset_timer;
void Initialize(void);
void LeaveBootloader(void);