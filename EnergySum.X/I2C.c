/*
 * Important: MSSP2 is reserved for I2C, MSSP1 for SPI
 * 
 * 317/491:
 * Accessible SSP registers:
 * MSSPxSTAT    359
 * MMSPxCON1    360
 * MSSPxCON2    362
 * MSSPxCON3    363
 * MSSPxBUF     365
 * 
 * TRIS, PORT, ANSEL:               154
 * I2C typical sequence:            349
 * MSSP registers:                  365
 * MSSPx overview:                  313
 * I2C mode:                        325
 * I2C master mode:                 343
 * I2C start condition:             345
 * I2C master mode transmission:    347
 * I2C master mode reception:       349
 * SSPxCLKPPS, DATPPS, SSPPS:       162
 */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "SPI.h"


/*
 * Initializes I2C master mode on MSSP2 register for PIC16F18345
 * SCL pin: RC3 (pin 7)
 * SDA pin: RC6 (pin 8)
 */
void I2C_MasterInit(void){
    INTCONbits.GIE = 0;             //Disable interrupts for the duration of the setup (not sure if necessary)
              
    //Change RC3 and RC6 to digital pins, 157
    ANSELCbits.ANSC3 = 0;
    ANSELCbits.ANSC6 = 0;
    // 325/491: 30.4.3: "When selecting any I2C mode, the SCL and SDA pins should be set by the user to inputs..."
    TRISCbits.TRISC3 = 1;           //Set RC4 (MISO) to input
    TRISCbits.TRISC6 = 1;           //Set RC4 (MISO) to input   
    PMD4bits.MSSP2MD = 0;           //Enable MSSP2 module (default is also 0 though), 170
    //p. 359
    SSP2STATbits.SMP = 1;
    SSP2STATbits.CKE = 0;   
    //Configuring pins, see 361/491 (note 2)
    PPSLOCK = 0;                    //Enable writing (also 0 by default) 164    
    //p. 325 Note: SDA input: SSPDATPPS, SCL input: SSPCLKPPS, outputs: RxyPPS, see p. 162
    RC6PPS = 0b11011;   //set RC6 to SDA2
    SSP2DATPPS = 0b10110;               
    RC3PPS = 0b11010;   //set RC3 to SCK2
    SSP2CLKPPS = 0b10011;   
    //p. 360
    SSP2CON1 = 0b00101000;   
    //see frequency is FOSC/((SSP2ADD+1)*4), 357
    SSP2ADD = 254;   
    //p. 362
    SSP2CON2 = 0b0;   
    //p. 363, also 325 30.4.4 SDA hold time
    SSP2CON3 = 0b0;
    SSP2IF = 0;
    SSP2IE = 1;
    PEIE = 1;    //101; enable peripherial interrupts
    GIE = 1;
}

#define ID_chip_address 0b001   //I2C address of the 24AA025E48 chip on-board
/*
 * Data sheet p. 345, initiates Start condition
 * BUT: 343: 2nd paragraph, in master mode interrupt generation is used
 */
void I2C_Start(void){
    PIR2bits.SSP2IF = 0;
    SSP2CON2bits.SEN = 1;
    while(PIR2bits.SSP2IF == 0);
    PIR2bits.SSP2IF = 0;
}

/*
 * p. 346/491, repeated start
 */
void I2C_Restart(void){
    SSP2CON2bits.RSEN = 1;
    while(SSP2CON2bits.RSEN == 1);
    PIR2bits.SSP2IF = 0;
}

/*
 * p. 351, stop condition
 */
void I2C_Stop(void){
    PIR2bits.SSP2IF = 0;
    SSP2CON2bits.PEN = 1;
    while(PIR2bits.SSP2IF == 0);
    PIR2bits.SSP2IF = 0;
}

/*
 * Structure:       350
 * Step by step:    349
 * SSP2CON2:        362
 * control_byte = 0b10100010
 */
void I2C_WriteByte(unsigned char control_byte, unsigned char data){
    //Write control bytes via I2C, see 347
    I2C_Start();                    //1-3. Start condition
    //4. ???
    SSP2BUF = control_byte;         //5.
    //6.
    while(PIR2bits.SSP2IF == 0);
    PIR2bits.SSP2IF = 0;            //8. SSP2IF is set, change it back
    //if(SSP2CON2bits.ACKSTAT == 0); //7. ACK bit received;
    SSP2BUF = data;       //9. Send memory address to be read (sequentially, that is, memory_address, ... + 1, ... + 2, etc., for data_length bits)
    //10.
    while(PIR2bits.SSP2IF == 0);
    PIR2bits.SSP2IF = 0;
}

/*
 * See 24AA... data sheet 12/32: 8.2, 8.3
 */
void I2C_ReadAddress(unsigned char control_byte_write, unsigned char control_byte_read, unsigned char memory_address, unsigned char* data, unsigned char data_length){
    unsigned char ind=0;
    I2C_WriteByte(control_byte_write, memory_address);
    //Start() condition; receive sequence: 349
    I2C_Start();                    //1-3.
    SSP2BUF = control_byte_read;    //4.
    while(PIR2bits.SSP2IF == 0);    
    PIR2bits.SSP2IF = 0;
    //if(SSP2CON2bits.ACKSTAT == 0);
     
    for(ind = 0; ind < data_length; ind++){
    SSP2CON2bits.RCEN = 1;              //8.
    while(PIR2bits.SSP2IF == 0);        //9.
    PIR2bits.SSP2IF = 0;                //10.
    data[ind] = SSP2BUF;
    SSP2STATbits.BF = 0;

    //Step 11
    SSP2CON2bits.ACKDT = 0;
    SSP2CON2bits.ACKEN = 1;

    //Step 12
    while(PIR2bits.SSP2IF == 0);
    //Step 13
    PIR2bits.SSP2IF = 0;
    }
    I2C_Stop();
}
 
void ReadGlobalAddress(unsigned char* data, unsigned char data_length){//data should be length 6 array
    I2C_MasterInit(); //initialize master temporarily on MSSP2
    unsigned char control_byte_write = 0b10100010;
    unsigned char control_byte_read = 0b10100011;
    unsigned char memory_address = 0xFA;
    I2C_ReadAddress(control_byte_write, control_byte_read, memory_address, data, data_length);
    SPI_Init(1, 1); //Back to SPI on MSSP2
}

/*
 * Initializes I2C slave mode on MSSP2 register for PIC16F18345, with slave address:
 * 
 * x S6 S5 S4 S3 S2 S1 S0           (7-bit address)
 * 
 * 
 * Description:                     327/491
 * I2C slave address: SSPxADD,      364/491
 * PIE2                             104
 * INTCON (interrupt control)       101
 * 
 * SCL pin: RC3 (pin 7)
 * SDA pin: RC6 (pin 8)
 */
void I2C_SlaveInit(unsigned char slave_address){    //slave_address of the form: x S6 S5 ... S0!
    GIE = 0;             //Disable interrupts for the duration of the setup
    //INTCONbits.PEIE = 0;            //Prevents other peripherials from interrupting set-up      
    //Change RC3 and RC6 to digital pins, 157
    ANSELCbits.ANSC3 = 0;
    ANSELCbits.ANSC6 = 0;   
    // 325/491: 30.4.3: "When selecting any I2C mode, the SCL and SDA pins should be set by the user to inputs..."
    TRISCbits.TRISC3 = 1;           //Set RC4 (MISO) to input
    TRISCbits.TRISC6 = 1;           //Set RC4 (MISO) to input    
    PMD4bits.MSSP2MD = 0;           //Enable MSSP2 module (default is also 0 though), 170     
    //p. 359
    SSP2STATbits.SMP = 0;//should be 0
    SSP2STATbits.CKE = 0;
    //Configuring pins, see 361/491 (note 2)
    PPSLOCK = 0;                    //Enable writing (also 0 by default) 164   
    //Set 7-bit address
    unsigned char address = slave_address;
    address <<= 1;              //364/491
    SSP2ADD = address;      
    //p. 325 Note: SDA input: SSPDATPPS, SCL input: SSPCLKPPS, outputs: RxyPPS, see p. 162
    RC6PPS = 0b11011;   //set RC6 to SDA2
    SSP2DATPPS = 0b10110;  
    RC3PPS = 0b11010;   //set RC3 to SCK2
    SSP2CLKPPS = 0b10011;   
    //p. 360
    SSP2CON1 = 0b00110110;  
    //p. 362
    SSP2CON2 = 0b0; //default settings
    SSP2CON3 = 0b0;
    //p. 363, also 325 30.4.4 SDA hold time
    SSP2IF = 0;
    SSP2IE = 1;            //Enable SSP2 interrupt  
    PEIE = 1;    //101; enable peripherial interrupts
    GIE = 1;     //enable global interrupts
}
