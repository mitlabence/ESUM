#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#define CS_DAC LATCbits.LATC0
#define CS_MDAC LATCbits.LATC1
#define MDAC_WriteA 0b00000001      //write command bits for AD5429 channel A
#define MDAC_WriteB 0b00000100      //write                         channel B
#define MDAC_ReadA  0b00000010      //read                          channel A
#define MDAC_ReadB  0b00000101      //read                          channel B
#define MDAC_NOP    0b00000000      //NOP

/*
 * SSPxSTAT register: 359/491
 * Other SSP registers: below
 */

/*
 * Initializes a master SPI unit of PIC16F18345 on MSSP1.
 * MOSI: Pin 5  (RC5)
 * MISO: Pin 6  (RC4)
 * SCK:  Pin 14 (RC2)
 * CS1:  Pin 15 (RC1)(CSMDAC)
 * CS2:  Pin 16 (RC0)(CSDAC)
 * 
 * Settings:
 * clock = FOSC/4
 * Transmit on transition from active to idle clock state (used for ODAC by default and for MDAC after sending appropriate command)
 * Default for MDAC is on falling edge (0); default for MDAC is rising edge (1); it is easier to set the MDACs by sending SCLK=1, see 20/28
 */
void SPI_Init(unsigned char clock_mode, unsigned char CKE){
    INTCONbits.GIE = 0;             //Disable interrupts for the setup
    
    ANSELC= (ANSELC & 0b11001000);  //Change RC0, RC1, RC2, RC4, RC5 to digital pins, not changing any other RCx pins
    LATC  = (LATC & 0b11100011);    //Change RC0, RC1 outputs to 1, RC2, RC4, RC5 to 0
    TRISCbits.TRISC4 = 1;           //Set RC4 (MISO) to input
    CS_DAC = 1;
    CS_MDAC = 1;             //Set CS pins to inactive high before activating them
    LATC = (LATC & 0b11001011);     //Set RC2, RC4, RC5 to 0 before activating them
    TRISC = (TRISC & 0b11011000);   //Set RC0, RC1 (CS), RC2 and RC5 to output   
    
    PMD4bits.MSSP1MD = 0;           //Enable MSSP1 module (used for SPI) (optional?)
    //Configuring pins, see 361/491 (note 2)
    PPSLOCK = 0;                    //Enable writing
    SSP1DATPPS = 0b00010100;        //162/491: set DAT input as RC4.
    RC5PPS = 0b00011001;            //Set RC5 pin output source as SDO1 (see 163/491)
    RC2PPS = 0b00011000;            //Set RC2 pin output source as SCK1
    PPSLOCK = 1;
    if(CKE == 0){
        SSP1STAT = 0b10000000;   
    }
    else{
        SSP1STAT = 0b11000000;
    }
    switch(clock_mode){
        case 0:
        SSP1CON1  = 0b00100011;
        break;
        case 2:
        SSP1CON1  = 0b00100001;
        break;
        case 3:
        SSP1CON1  = 0b00100010;
        break;
        case 1:                     //use Fosc/4 by default (case 1 is same as default then)
        default:                
        SSP1CON1  = 0b00100000;
    }
    
    INTCONbits.GIE = 1;
}

/*
 * Set the both CS pins according to the parameter setting:
 * 0: select offset DAC (CS_DAC to active low, CS_MDAC to high)
 * 1: select MDAC
 * 2: set both to inactive high
 * everything else is ignored
 */
void Set_CS(unsigned char setting){
    switch(setting){
        case 0:
            CS_DAC = 0;
            CS_MDAC = 1;
            break;
        case 1:
            CS_DAC = 1;
            CS_MDAC = 0;
            break;
        case 2:
            CS_DAC = 1;
            CS_MDAC = 1;
            break;
    }
}

/*
 * !CS needs to be controlled by the code!
 * Takes 1 byte of data, transmits it via SPI and returns the answer.
 */
unsigned char SPI_IO_Byte(unsigned char data){
    unsigned char read_data;
    //360, 319
    do{
        SSP1CON1bits.WCOL = 0;
        SSP1BUF = data;
    }while(SSP1CON1bits.WCOL);
    while(SSP1IF == 0); //wait until transmission complete
    read_data = SSP1BUF;
    SSP1IF = 0;
    return read_data;
}

/*
 * !CS needs to be controlled by the code!
 * Takes array of 8-bit commands to be sent
 * Fills the parameter array with answers from slave
 * Modifying argument array: https://zhu45.org/posts/2017/Jan/08/modify-array-inside-function-in-c/
 */
void SPI_IO(unsigned char *data_array, unsigned char length){
    int i;
    unsigned char received_data;
    /*if(data_array[0] == 0b00000110){
        data_array[2] = 1;
    }*/
    for(i = 0; i < length; i++){
        received_data = SPI_IO_Byte(data_array[i]);
        data_array[i] = received_data;
    }
}