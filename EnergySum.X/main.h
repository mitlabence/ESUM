#ifndef MAIN_HEADER
#define	MAIN_HEADER

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "I2C.h"
#include "ADC.h"
#include "SPI.h"
#include "ODAC.h"
#include "MDAC.h"
#include "common.h" //flash memory-related functions are also in this one, because otherwise compiler throws errors... Made me swear quite a lot until figured it out.

/* See PIC16F18345 data sheet:
 * Config Words:    63/491
 * I2C slave mode:  327
 * SSP2STAT:        359
 * SSP2CON1:        360
 * SSP2CON2:        362
 * I2C Slave reception: 329
 * I2C Slave transmission: 334
 */

/*
 * Structure of memory:
 * 0-23:    MDAC values
 * 24:      MDAC flags (bits 7-4: MDAC number, bit 0: write single MDAC, bit 1: write all MDACs, bit 2: read all MDACs   
 * 25-26:   Offset DAC value (12-bit)
 * 27:      Offset DAC flag (bit 0: write, bit 1: read)
 * 28-29:   ADC input channel (bit 10, 0 if baseline pin, 1 if NREF pin) and ADC value (bits 9, 8, ..., 0)
 * 30:      ADC flag (bit 0: ADC measure, bit 1: input channel (0 if baseline pin, 1 if NREF pin))
 * possibly 31:      Testpulser value (code for frequency setting, maybe)
 * 32:      Testpulser flag
 * 33:      Baseline flag (bit 0: set baseline)
 * 34:      UID flag (bit 0: UID is read out)
 * 35:      UID storage 
 * 36:
 * 37:      I2C address flag (bit 0: if 1, i2c reinitializes with new I2C address)
 * 38:      I2C storage (set address here first, then set flag, see above, to update)
 * 100:     (arbitrary) used to empty global i2c address from SSP2BUF (memory[...] = SSP2BUF needed to clear SSP2BUF, BF flag etc.)
 */

#define MDAC_flag    24
#define ODAC_flag    0x20
#define ODAC_MSB     0x21
#define ODAC_LSB     0x22
#define ADC_flag     0x30 //bit 0: ADC measure, bit 1: 0 if baseline pin, 1 NREF pin should be input
#define ADC_MSB      0x31 //ADC_LSB is obviously 0x32
#define UID_flag     0x40  //flag for reading out UID
#define UID_store    0x41  //first byte of 6-byte UID stored by 24AA... chip
#define TP_flag      0x50  //Also defined in testpulser.c
#define BL_flag      0x51  //if bit 0 is 1: baseline setting is started 
#define LED_flag     0x52  //flag for switching LED status
#define I2C_flag     0x53  //i2c flag, if bit 0 is 1, reinitializes i2c with address stored in memory[I2C_store]
#define I2C_store    0x54  //i2c address can be modified here, and setting memory[I2C_flag] to 0b1 causes I2C get reinitialized with new address. IMPORTANT: check raspberry pi i2cdetect -y 1 function to see valid values!
#define global_address_store 0x55 //i2c global address is written here, no particular use, only to clear SSP2BUF

#define version_register 0xff   //memory[] index

#define version_number 0x07     //arbitrary (should be consequent) version number

#define i2c_default_address 0x2e        //7-bit i2c address assigned during power-on; should be 0x1e for level 1 ESUM module firmware, 0x2e for level 2 firmware

extern unsigned char memory[256];

#endif

/*
 *  Configuring firmware:
 *  1. check if _XTAL_FREQ (external frequency) defined in ADC.h is correct
 *  2. If module is level 1 (12 MDACs, 23 channels, 1 testpulser), set the following:
 *      i)   in main.h: i2c_default_address 0x1e
 *      ii)  in MDAC.h: daisy_chain_length  12
 *     If module is level 2 (4 MDACs, 8 channels, 1 fixed tespulser), set:
 *      i)   in main.h: i2c_default_address 0x2e
 *      ii)  in MDAC.h: daisy_chain_length  4
 *  3. Check version_number. If there have been significant notifications, increment by one.
 * 
 * If a hex fuke us nade fir uploading to a module, make sure the following are set (right click on project folder -> set configuration -> custom):
 * XC8 linker:  Runtime: Format hex file for download
 *              Additional options: codeoffset 0x1000 (check if bootloader still has this value, i.e. first half of program flash memory is protected!)
 */