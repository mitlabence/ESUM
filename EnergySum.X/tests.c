#include "tests.h"
/*
 * Simply call in while(1) loop in main() (SPI etc. setup needed!)
 */
void test_WriteSingleMDAC(void){
        for(unsigned char i = 0; i < 24; i++){
            memory[i] = 0xff;
        }
        for(unsigned char i = 1; i < 25; i++){
            memory[24] = (i << 3);
            memory[24] |= 0b00000001;
            SPI_Process();
            __delay_ms(10);
        }
        __delay_ms(1000);
        
        for(unsigned char i = 0; i < 24; i++){
            memory[i] = 0x7f;
        }
        for(unsigned char i = 1; i < 25; i++){
            memory[24] = (i << 3);
            memory[24] |= 0b00000001;
            SPI_Process();
            __delay_ms(10);
        }
        __delay_ms(1000);
        
        for(unsigned char i = 0; i < 24; i++){
            memory[i] = 0x00;
        }
        for(unsigned char i = 1; i < 25; i++){
            memory[24] = (i << 3);
            memory[24] |= 0b00000001;
            SPI_Process();
            __delay_ms(10);
        }
        __delay_ms(1000);
}
 
void testODAC(void){
    uint16_t odac_val = 0x0345;
    Set_CS(0);
    ODAC_IO(VOL_DAC0_ADDRESS, 1, odac_val);
    Set_CS(2);
    Set_CS(0);
    odac_val = ODAC_IO(VOL_DAC0_ADDRESS, 0, 0);
    Set_CS(2);
    odac_val += 1;
    /*
    memory[ODAC_flag] = 0b01;
    if(memory[ODAC_flag] > 0){
        if(memory[ODAC_flag] & 0b01){
    
    odac_val |= odac_msb;
    odac_val <<= 8;
    odac_val |= odac_lsb;
    Set_CS(0);
    ODAC_IO(VOL_DAC0_ADDRESS, 1, odac_val);
    Set_CS(2);
        }}
    memory[ODAC_flag] = 0b10;
    __delay_ms(100);
    
    if(memory[ODAC_flag] > 0){
        if(memory[ODAC_flag] & 0b10){
    Set_CS(0);
    odac_val = ODAC_IO(VOL_DAC0_ADDRESS, 0, 0);
    Set_CS(2);
    memory[ODAC_LSB] = (odac_val & 0xff);
    odac_lsb =  (odac_val & 0xff);
    odac_val >>= 8;
    memory[ODAC_MSB] = (odac_val & 0xff);
    odac_msb = (odac_val & 0xff);
    memory[ODAC_flag] = 0;
        }}*/
     
}