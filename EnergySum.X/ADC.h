#ifndef ADC_HEADER
#define	ADC_HEADER

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdint.h>

#define _XTAL_FREQ 32000000

void ADC_SelectChannel(unsigned char mode);
void ADC_Configure(unsigned char mode);
uint16_t ADC_Measure(void);

#endif

