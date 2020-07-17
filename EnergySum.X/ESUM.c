/*
 * config not needed when uploaded with 
// CONFIG1
 */
#pragma config FEXTOSC = OFF    // FEXTOSC External Oscillator mode Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with 2x PLL (32MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; I/O or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config MCLRE = ON       // Master Clear Enable bit (MCLR/VPP pin function is MCLR; Weak pull-up enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config WDTE = OFF       // Watchdog Timer Enable bits (WDT disabled; SWDTEN is ignored)
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bits (Brown-out Reset enabled, SBOREN bit ignored)
#pragma config BORV = LOW       // Brown-out Reset Voltage selection bit (Brown-out voltage (Vbor) set to 2.45V)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (The PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a Reset)
#pragma config DEBUG = ON      // Debugger enable bit (Background debugger disabled)

// CONFIG3
#pragma config WRT = OFF        // User NVM self-write protection bits (Write protection off)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/VPP must be used for programming.)

// CONFIG4
#pragma config CP = OFF         // User NVM Program Memory Code Protection bit (User NVM code protection disabled)
#pragma config CPD = OFF        // Data NVM Memory Code Protection bit (Data NVM code protection disabled)


#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"


unsigned char memory[256];
bool next_is_mem_addr = false;
bool first_read_back = true; //helpful variable: when reading back multiple values, D_nA will be 1, which normally would correspond to situation when global address has just been received.
unsigned char memory_address = 0;  //current index of memory[]
 
 /*
  * interrupt enable: 101/491
  * SSPxSTAT: 359
  * I2C transmit: 333
  * TODO:   during a read command, D_nA gets checked. If there has been an I2C address then stop, then i2c address with read command, this bit will be 0... It is assumed that each command ends with data, which might not be true!
  *           
  */
 void __interrupt() isr(void){
     if(SSP2IF == 1){
         if(SSP2STATbits.R_nW == 1 ){//Read command
            if((first_read_back == 1) && (SSP2STATbits.D_nA == 1)){//1. before each command, it is assumed that a data byte was transmitted first in either way -> D_nA bit is high right after receiving global address
                memory[global_address_store] = SSP2BUF;
                SSP2IF = 0;
            }
            else{//2. - N. read back memory and increment array pointer
                SSP2BUF = memory[memory_address];
                memory_address++;
                SSP2CON1bits.CKP = 1;
                first_read_back = 0; //the first byte has been read back. D_nA will be 1, but should not run 
                SSP2IF = 0;
            }
         }
         else{//Write
             if(SSP2STATbits.D_nA == 1){
                 if(next_is_mem_addr){//receiving memory[] address to be written to
                     memory_address = SSP2BUF;
                     next_is_mem_addr = false;
                     SSP2IF = 0;
                 }
                 else{
                     memory[memory_address] = SSP2BUF;
                     memory_address++;
                     SSP2IF = 0; 
                 }
             }
             else{//Read global address from buffer
                memory_address = SSP2BUF;
                next_is_mem_addr = true; //next byte received will be the memory[] address
                if(SSP2CON2bits.SEN == 1){//if clock stretching enabled, release SCL
                    SSP2CON1bits.CKP = 1;
                }
                SSP2IF = 0;
             }
         }
    if(SSP2STATbits.P == 1){
        first_read_back = 1; //if stop detected, clear "first byte read back" (i. e. next read command's first byte will also be detected as first byte read back)
    }
     }
     
 } 
 
 /*
 * MDAC_flag: bits 7-3: MDAC channel, bit 0: write single, bit 1: write all, bit 2: read all
 * MDAC channel should be in range [1, 24]!!!
 * ODAC_flag: bit 0: write, bit 1: read
 */

void SPI_Process(void){
    unsigned char mdac_flag_bits = memory[MDAC_flag] & 0b111; //ignore MDAC channel bits to check for tasks
    if(mdac_flag_bits > 0){
        
        if(mdac_flag_bits & 0b010){//write all bit is set
            MDAC_LoadUpdate(MDAC_1);
            memory[MDAC_flag] &= 0b11111101;//clear flag
        }
        if(mdac_flag_bits & 0b001){//write single bit is on
                unsigned char MDAC_index = memory[MDAC_flag]&(0b11111000);
                unsigned char mdi;
                MDAC_index >>= 3;
                mdi = MDAC_index;
                mdi--;
                Set_CS(1);
                MDAC_LoadUpdateSingle(MDAC_index, memory[mdi]); //MDAC_index goes from 1 to 24, but memory[0] contains value for MDAC 1
                Set_CS(2);
                memory[MDAC_flag] &= 0b11111110;//clear flag  
        }
        if(mdac_flag_bits & 0b100){//read all
            MDAC_Read();
            memory[MDAC_flag] &= 0b11111011;//clear flag
        }
    }
    if(memory[ODAC_flag] > 0){
        MDAC_SendToLast(MDAC_ControlWord, MDAC_disable_command); //disable MDAC, otherwise it would interfere in SPI communication
        
        if(memory[ODAC_flag] & 0b01){//Write command
            uint16_t odac_val = 0;
            odac_val |= memory[ODAC_MSB];
            odac_val <<= 8;
            odac_val |= memory[ODAC_LSB];
            Set_CS(0);
            ODAC_IO(VOL_DAC0_ADDRESS, 1, odac_val);
            Set_CS(2);
            memory[ODAC_flag] &= 0b11111110;//clear flag
        }
        if(memory[ODAC_flag] & 0b10){//Read
            uint16_t odac_val = 0;
            Set_CS(0);
            odac_val = ODAC_IO(VOL_DAC0_ADDRESS, 0, 0);
            Set_CS(2);
            memory[ODAC_LSB] = (odac_val & 0xff);
            odac_val >>= 8;
            memory[ODAC_MSB] = (odac_val & 0xff);
            memory[ODAC_flag] &= 0b11111101;
        }
            Set_CS(1);
            MDAC_Init(0, 0, 1); //TODO: sending  MDAC_SendToLast(MDAC_ControlWord, MDAC_enable_command); did not work, why?
            Set_CS(2);
    }
    
}

/*
 * Only purpose of I2C_Process() is to update i2c slave if memory[I2C_flag] is 0b1.
 */
void I2C_Process(void){
    if((memory[I2C_flag] & 0b1) == 0b1){
        SSP2CON1bits.SSPEN = 0; //Disable SSP2 (I2C) module
        I2C_SlaveInit(memory[I2C_store]);
        memory[I2C_flag] &= 0b11111110; //clear last bit
    }
}

/*
 * ADC_flag:      ADC flag (bit 0: ADC measure, bit 1: 0 if baseline pin, 1 if NREF pin should be input)
 * BL_flag:       if == 1, baseline will be set and the LSB cleared.
 */
void ADC_Process(void){
    if(memory[ADC_flag] > 0){
        unsigned char mode = (memory[ADC_flag] & 0b00000010); //mode should be 0 for baseline, 1 for NREF
        mode >>= 1;
        uint16_t adc_value;
        ADC_Configure(mode);
        __delay_ms(200);
        adc_value = ADC_Measure();
        __delay_ms(1000);
        memory[ADC_MSB+1] = (adc_value & 0xff);
        memory[ADC_MSB] = adc_value >> 8;
        memory[ADC_flag] &= 0b11111110;
    }
    if(memory[BL_flag] > 0){
        if(memory[BL_flag] == 0b1){
            SetBaseline();
            memory[BL_flag] &= 0b11111110;
        }
    }
}

void Testpulser_Do(void){
    /*
     * Equation: see 194
     */
    T6CONbits.T6CKPS = 0b11;        //Prescaler: 64
    T6CONbits.T6OUTPS = 0;     //Postscaler
    PR6 = 255;
    
    TRISBbits.TRISB6 = 0;
    
    PIE2bits.TMR6IE = 0;
    TMR6ON = 1; //start timer
    while(1){
        if(memory[TP_flag] == 0){
            break;
        }
        LATB = 0b01000000;
        LATB = 0;
        while(PIR2bits.TMR6IF == 0);
        PIR2bits.TMR6IF = 0;
    }
    TMR6ON = 0;
}

/*
 * TP_flag: to be implemented, now: bit 0: testpulser on
 */
void Testpulser_Process(void){
    if(memory[TP_flag] > 0){
        Testpulser_Do();
        memory[TP_flag] = 0;
    }
}

void Misc_Process(void){//UID, LED functions
    //reads out 24AA025E48 chip unique address when memory[UID_flag] is set to 1
    if(memory[UID_flag] == 0b1){
            memory[UID_flag] = 0;
            INTCONbits.GIE = 0;//Disable interrupts for the time being
            SSP2IF = 0; //TODO: maybe not needed?
            unsigned char data[6] = {0, 0, 0, 0, 0, 0};
            unsigned char data_length = 6;
            ReadGlobalAddress(data, data_length);
            for(unsigned char i = 0; i < data_length; i++){
                memory[UID_store+i] = data[i];
            }
            I2C_SlaveInit(memory[I2C_store]); //global interrupt will be enabled by this function

            
        }
    if(memory[LED_flag] != 0b0){//LED settings: 1-3 is switching LED 1-3, anything else switches them all
        switch(memory[LED_flag]){
            case 1:
                LATAbits.LATA2 ^= 1;
                break;
            case 2:
                LATAbits.LATA4 ^= 1;
                break;
            case 3:
                LATAbits.LATA5 ^= 1;
                break;
            default://switch output status for all three LED pins
                LATAbits.LATA2 ^= 1;
                LATAbits.LATA4 ^= 1;
                LATAbits.LATA5 ^= 1;
                break;
                
        }

        memory[LED_flag] = 0;
    }
    if (memory[reset_flag] != 0x00){//if reset_flag is 0, nothing happens; if it is 1, only reboot happens; if it is 2, reboot into bootloader happens.
        if(memory[reset_flag] == 0x02){
            flash_memory_set_bootloader_flag();
            __delay_ms(10);
        }
        asm("RESET");
    }
}

void main(void) {
    __delay_ms(100); //wait a little for the power-on voltage instabilities to settle
    memory[I2C_store] = i2c_default_address;
    memory[reset_flag] = 0x00; //initialize flag to 0 to avoid accidental resets.
    //Set up LED pins
    TRISAbits.TRISA2 = 0;       //LED1
    TRISAbits.TRISA4 = 0;       //LED2
    TRISAbits.TRISA5 = 0;       //LED3

    //set testpulser pins to output, and low
    TRISBbits.TRISB6 = 0;
    LATBbits.LATB6 = 0;
    
    LATAbits.LATA2 = 1;
    LATAbits.LATA4 = 1;
    LATAbits.LATA5 = 1;

    /*
    for(unsigned char i = 0; i < 64; ++i){
        LATAbits.LATA2 ^= 1;
        LATAbits.LATA4 ^= 1;
        LATAbits.LATA5 ^= 1;
        __delay_ms(300);
    }
     */
    /*
     * Initialize SPI master on MSSP1 register
     * 
     * By default, the active clock edge is the falling edge for the MDACs, the rising edge for the offset DAC.
     * Initialize SPI to work with the MDACs
     * Change the MDACs to rising edge
     * Reinitialize SPI to work with MDACs and offset DAC.
     */
    SPI_Init(1,0);
    Set_CS(1);
    MDAC_Init(0, 0, 1);
    Set_CS(2);
    SPI_Init(1, 1);
    
    //Initialize I2C slave on MSSP2 register with default i2c address. It can be changed, see I2C_flag definition.
    SSP2CON1bits.SSPEN = 0; //Disable SSP2 (I2C) module
    I2C_SlaveInit(memory[I2C_store]);

    
    //store firmware version
    memory[version_register] = version_number;
    
    ODAC_SelectReference(1);//Set up ODAC with internal band gap as reference
    ODAC_SetGain(1);        
    
    //Write "MDAC" to help when looking at i2cdump
    memory[0x1c] = 0x4d;
    memory[0x1d] = 0x44;
    memory[0x1e] = 0x41;
    memory[0x1f] = 0x43;
    //Write "ODAC"
    memory[0x2c] = 0x4f;
    memory[0x2d] = 0x44;
    memory[0x2e] = 0x41;
    memory[0x2f] = 0x43;
    //Write "ADC"
    memory[0x3c] = 0x41;
    memory[0x3d] = 0x44;
    memory[0x3e] = 0x43;
    //Write "UID"
    memory[0x4c] = 0x55;
    memory[0x4d] = 0x49;
    memory[0x4e] = 0x44;
    
    while(1){
        asm("CLRWDT");              //Clear watchdog timer
        SPI_Process();            //Reads/updates MDACs on MSSP1 
        I2C_Process();            //Reinitializes slave if needed
        ADC_Process();
        Testpulser_Process();     //Produces testpulses
        Misc_Process();
     }
}


/*
 * Urgent:
 * TODO: in I2C_SlaveInit(), SSP2CON1 has CKP set to 1. In bootloader, setting it to 0 twice (reinitialization) caused problems, so hopefully this is the correct way to do it.
 * TODO: I2C_SlaveInit does not work in initialization step, only peripherial interrupt enables work. Why??
 * TODO: read out global UID does not seem to work: after setting flag to 1, i2cdetect sees random addresses, sometimes also 0x51 (UID chip), sometimes not.
 * TODO: 142/491: input mode always analog! If a pin is used as digital only (output and input, genereal purpose), then this could cause problems!
 * TODO: ADC: to get value corresponding to 0 V, one could on start-up set the input channel to the fixed voltage reference (FVR), 244/491, do a measurement, and from this value, calculate the 0V.
 * TODO: Use flash_memory_set_flag() (User ID) to set bootloader flag.
 * TODO: Rewrite I2C_Process to avoid using MasterInit.
 * Not so urgent:
 * TODO: implement a single MDAC channel update method where 3 bytes can be used (1 or 0 for each bit) to decide if that MDAC channel needs to be updated (generalization of update single MDAC)
 * TODO: need a way to set baseline value which the ADC will compare to (see SetBaseline(), if ADC_val < reference_adc_val, then ...), so that we can set the baseline value for each board independently. It should also survive a power-out!
 * Idea: use internal reference voltage!
 * 
 */

/*
 * I2C slave reinitialization problem:
 * I2C slave is initialized in bootloader. When program initializes I2C slave on same MSSP module (MSSP2), i2c data and clock are held at constant (nothing visible in oscilloscope).
 * When in program, i2c slave is not reinitialized, only interrupts are enabled for it, everything seems to be working.
 * 
 * Idea: Check initialization step-by-step. Maybe MSSP needs to be turned off first, maybe something else...
 * 
 * There might be a link between this issue and the I2C_Process() problem, where MasterInit is needed instead of simply SlaveInit.
 * 
 * Solution:
 * 
 * 317: To reset or reconfigure the SPI module, clear the SSPEN bit, re-initialize the SSPxCONy registers and then set the SSPEN bit. The pins need to be set up as well!
 */
