// CONFIG1
#pragma config FEXTOSC = OFF    // FEXTOSC External Oscillator mode Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with 2x PLL (32MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; I/O or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config MCLRE = ON       // Master Clear Enable bit (MCLR/VPP pin function is MCLR; Weak pull-up enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config WDTE = SWDTEN    // WDT controlled by the SWDTEN bit in the WDTCON register
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bits (Brown-out Reset enabled, SBOREN bit ignored)
#pragma config BORV = LOW       // Brown-out Reset Voltage selection bit (Brown-out voltage (Vbor) set to 2.45V)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (The PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a Reset)
#pragma config DEBUG = OFF      // Debugger enable bit (Background debugger disabled)

// CONFIG3
#pragma config WRT = HALF        // User NVM self-write protection bits (Write protection for 0000h to 0FFFh)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/VPP must be used for programming.)

// CONFIG4
#pragma config CP = OFF         // User NVM Program Memory Code Protection bit (User NVM code protection disabled)
#pragma config CPD = OFF        // Data NVM Memory Code Protection bit (Data NVM code protection disabled)


#include <xc.h>
#include "main.h"
#include "flash.h"
#include "i2c.h"
#include "typedef.h"

#define I2C_SLAVE_ADDRESS 0x1f //convention: use 0x1f for bootloader of all stages (0x2f does not work due to CRC mismatch, see i2c.c to-do comment at lookup_table definition), 0x1e and 0x2e for level 1 and level 2 ESUM firmware, correspondingly!
//lookup table for CRC error-detecting algorithm
const unsigned char lookup_table[256] = {0,29,58,39,116,105,78,83,232,245,210,207,156,129,166,187,205,208,247,234,185,164,131,158,37,56,31,2,81,76,107,118,135,154,189,160,243,238,201,212,111,114,85,72,27,6,33,60,74,87,112,109,62,35,4,25,162,191,152,133,214,203,236,241,19,14,41,52,103,122,93,64,251,230,193,220,143,146,181,168,222,195,228,249,170,183,144,141,54,43,12,17,66,95,120,101,148,137,174,179,224,253,218,199,124,97,70,91,8,21,50,47,89,68,99,126,45,48,23,10,177,172,139,150,197,216,255,226,38,59,28,1,82,79,104,117,206,211,244,233,186,167,128,157,235,246,209,204,159,130,165,184,3,30,57,36,119,106,77,80,161,188,155,134,213,200,239,242,73,84,115,110,61,32,7,26,108,113,86,75,24,5,34,63,132,153,190,163,240,237,202,215,53,40,15,18,65,92,123,102,221,192,231,250,169,180,147,142,248,229,194,223,140,145,182,171,16,13,42,55,100,121,94,67,178,175,136,149,198,219,252,225,90,71,96,125,46,51,20,9,127,98,69,88,11,22,49,44,151,138,173,176,227,254,217,196};

unsigned char i2c_status;
unsigned char i2c_control_word;
ADDRESS flash_address;
unsigned char flash_address_index; //used to remember which byte needs to be written to during I2C communication
unsigned char flash_buffer[64];
unsigned char flash_buffer_index;
unsigned char reset_timer; //initialize to 0; once it is 1, the timer is reset

//the interrupt flag is at 0x004 in the PIC, i.e. bootloader space; half the memory is protected. This function forwards the flag to the unprotected 0x1004 (the firmware should have offset 0x1000).
void  __interrupt() service_isr(){
    PCLATH = 0b0010000; //56/491 Figure 4-3.; want to jump to 0x100_.
    asm("GOTO 0x0004;"); //see 403/491 on description of GOTO
}	

 /*
  * Program memory map:  28/491
  * 
  * IMPORTANT: a watchdog timer makes sure to do a reset if the program gets stuck. This holds for both this bootloader and any uploaded program! asm("CLRWDT") needs to be used regularly.
  */
void main(void) {
    __delay_ms(100); //wait for voltages to stabilize
    Initialize();
    I2C_SlaveInit(I2C_SLAVE_ADDRESS);
    if(flash_memory_read(bootloader_flag_address, 1) == 1){//bootloader flag: 1 to leave bootloader, 0 to stay.
        LeaveBootloader();
    }
    reset_timer = 1; //for some reason, if timer is not reset in beginning, TMR0IF gets set instantly, leading to a waiting time for i2c of 0.
    while(1){
        I2C_Communicate();
        if(reset_timer){
            TMR0H = 0;
            TMR0L = 0;
            reset_timer = 0;
        }
        if(TMR0IF){//Waiting time is up; IMPORTANT: if no firmware is present, a restart is still needed to get back to bootloader again.
            TMR0IF = 0;
            LeaveBootloader(); 
        }
        asm("CLRWDT"); //clear watchdog timer
    }
    return;
}

//Initialize LED pins for bootloader
void Initialize(void){
    PCLATH = 0b0;
    INTCONbits.GIE = 0; //disable global interrupt while bootloader is running
    ANSELA = 0; //no need for analog inputs on register A
    //Set up pins
    TRISAbits.TRISA2 = 0;       //LED1
    TRISAbits.TRISA4 = 0;       //LED2
    TRISAbits.TRISA5 = 0;       //LED3
    
    //only LED 1 should be on
    LATAbits.LATA2 = 0;
    LATAbits.LATA4 = 1;
    LATAbits.LATA5 = 1;
    
    //Initialize T0 timer module (276-282/491)
    /*
     * Oscillator frequency: 31 kHz
     * Counter: 16-bit (65536)
     * Prescaler: 1-2^15 (32768)
     * Postscaler: 1-16
     * --> if pre- and postscalers 1:1, will have a period of ~2.1 s.
     * Some useful settings:
     * 4.2 sec timeout (for testing):
     * T0CKPS = 0b0001, T0OUTPS = 0b0000
     * 8.96 ~ 9 min timeout: prescaler 1:256
     * T0CKPS = 0b1000, T0OUTPS = 0b0000
     * ~27 min timeout: 10 min timeout + postscaler 1:3
     * T0CKPS = 0b1000, T0OUTPS = 0b0010
     * ~54 min timeout: 10 min timeout + postscaler 1:6
     * T0CKPS = 0b1000, T0OUTPS = 0b0101
     */
    PIE0bits.TMR0IE = 0;
    OSCENbits.LFOEN = 1; //enable 31kHz oscillator
    T0CON0bits.T016BIT = 1;
    T0CON0bits.T0OUTPS = 0b0010;
    
    T0CON1bits.T0CS = 0b100;
    T0CON1bits.T0ASYNC = 0;
    T0CON1bits.T0CKPS = 0b1000; //prescaler
   
    T0CON0bits.T0EN = 1;
    TMR0IF = 0;
    //T0CON0 = 0b10010000;    //280
    //T0CON1 = 0b10000000;    //281
    reset_timer = 0;
    
    //Initialize Watchdog timer
    WDTCONbits.WDTPS = 0b01110; //approx. 16 s without CLRWDT until timeout. This should avoid watchdog-resets during an upload process. See: 119/491
    WDTCONbits.SWDTEN = 1; //start watchdog timer
}

void LeaveBootloader(void){
    asm("CLRWDT"); //clear watchdog timer to avoid being interrupted by its reset
    T0CON0bits.T0EN = 0; //Disable timer
    OSCENbits.LFOEN = 0; //Turn off oscillator
    TMR0IF = 0;
    //datasheet 317: to reconfigure SPI (or, in this case, the SSP module), need to clear SSPEN bit.
    flash_memory_set_bootloader_flag(0); //on restart, stay in bootloader
    SSP2IF = 0;
    PCLATH = 0b0010000; //56/491 Figure 4-3.; want to jump to 0x100_.
    asm("GOTO 0x0000;"); //see 403/491 on description of GOTO
}