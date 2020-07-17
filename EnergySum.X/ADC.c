
#include <xc.h>
#include "ADC.h"

/*
 * Internal ADC functions for PIC16F18345
 * PMD:                     166/491
 * Fixed Voltage Ref.:      179/491
 * Temp. Ind. Module:       182/491
 * Procedure:               241/491
 * ADC control register:    244/491
 */

/*
 * Quick usage of ADC:
 * 1. Use ADC_Configure(mode)
 * 2. Use ADC_Measure() repeatedly
 */


/* 
 * USE ADC_Configure() to set up ADC!
 * This function selects the input pin for the ADC.
 * 
 * mode: selects analog input
 *  0:  RC7 (Baseline pin)
 *  1:  N_REF_Monitor pin
 *  2:  internal temperature indicator module (high range mode)
 *  3:  Fixed Voltage Reference (179/491) TODO it should be removed!
 */
void ADC_SelectChannel(unsigned char channel){
    switch(channel){
        case 0:
            TRISCbits.TRISC7 = 1;       //input (disable output)
            ANSELCbits.ANSC7 = 1;       //analog input enable
            ADCON0bits.CHS = 0b010111;  //see 244/491
            break;
        case 1:
            TRISBbits.TRISB7 = 1;
            ANSELBbits.ANSB7 = 1;
            ADCON0bits.CHS = 0b001111;  //see 244/491
            break;
        case 2:
            FVRCONbits.TSRNG = 1;   //High range
            FVRCONbits.TSEN = 1;
            ADCON0bits.CHS = 0b111101;  //see 244/491
            break;
        case 3:
            ADCON0bits.CHS = 0b111111; //244/491
            //FVRCON = 0b11000001; //enable FVR, set ADC FVR buffer gain to 1x (1.024 V)
            
            break;
            }
            
    }


/*
 * 1. and 2. steps of ADC measurement process, see 241/591
 * mode: selects analog input
 *  0:  RC7 (Baseline pin)
 *  1:  N_REF_Monitor pin
 *  2:  internal temperature indicator module (high range mode)
 */
void ADC_Configure(unsigned char mode){
    //1. Configure Port: see 157/491, notes
    ADC_SelectChannel(mode);
    //Set up Fixed Voltage Reference
    PMD0bits.FVRMD = 0;
    FVRCONbits.ADFVR = 1; //1.024 V internal reference voltage
    FVRCONbits.FVREN = 1;
    
    //2. Configure ADCON0 and ADCON1
    ADCON1bits.ADFM = 1;        //Right justified (6 MSB of ADRESH are 0)
    ADCON1bits.ADCS = 0b111;    //set ADCRC (dedicated RC oscillator) as clock
    ADCON1bits.ADNREF = 0;      //ground as reference-
    ADCON1bits.ADPREF = 0b11;   //Internal FVR as reference+
    
    //Enable ADC
    ADCON0bits.ADON = 1;
}

/*
 * (3) 4-7. (8) steps of ADC measurement process, see 241/591
 */
uint16_t ADC_Measure(void){
    uint16_t result;
    result = 0;
    //3. OPTIONAL: configure interrupt. For now, work with polling method.
    
    //4. Wait for acquisition, see 242/491
    __delay_us(3);
    //5. Set GO bit
    ADCON0bits.GO = 1;
    //6. Wait until conversion is complete
    while(ADCON0bits.GO == 1);
    //7. Read result (ASSUMING RIGHT JUSTIFIED SETTING)
    result |= ADRESH;
    result <<= 8;
    result |= ADRESL;
    //8. Clear interrupt flag if used
    return result;
}
