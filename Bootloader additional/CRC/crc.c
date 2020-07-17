/*
This code implements CRC-8, to be used in the bootloader for the energy sum modules.
Standards: See https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf page 22
SAE J1850 CRC:
Polynomial: 0x1D
Initial value: 0xFF
input data reflected: No
result data reflected: No
XOR value: 0xFF
Check: 0x4B
Magic check: 0xC4

Tests:
00 00 00 00                 -> 59
F2 01 83                    -> 37
0F AA 00 55                 -> 79
00 FF 55 11                 -> B8
33 22 55 AA BB CC DD EE FF  -> CB
92 6B 55                    -> 8C
FF FF FF FF                 -> 74

*/

#include <stdio.h>

typedef unsigned char crc;
#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))
#define POLYNOMIAL 0x1D
#define INITIAL_REMAINDER 0xFF
#define FINAL_XOR_VALUE 0xFF
crc crcTable[256];
void crcTableInit(void){
    crc remainder;
    for(unsigned char dividend = 0; dividend < 256; dividend++){
        remainder = dividend << (WIDTH - 8);
        for(unsigned char bit = 8; bit > 0; bit--){
            if (remainder & TOPBIT)//if remainder starts with 1
            {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            }
            else //if remainder starts with 0
            {
                remainder = (remainder << 1);
            }
        }
        crcTable[dividend] = remainder;
        printf("%u,", remainder);
        if(dividend == 0b11111111){//unsigned char would just increment and become 0 again; need to break the loop
            break;
        }
    }
}

crc CRC(const unsigned char *message, int message_length){
    unsigned char data;
    crc remainder = INITIAL_REMAINDER;
    for(unsigned char i = 0; i < message_length; i++){
        data = message[i] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }
    return (remainder ^ FINAL_XOR_VALUE);
}

int main(void){
    crcTableInit();
    unsigned char msg1[] = {0, 0, 0, 0};
    printf("%04x expected: 59\n", CRC(msg1, 4));

    unsigned char msg2[] = {0xF2, 0x01, 0x83};
    printf("%04x expected: 37\n", CRC(msg2, 3));

    unsigned char msg3[] = {0x0F, 0xAA, 0x00, 0x55};
    printf("%04x expected: 79\n", CRC(msg3, 4));

    unsigned char msg4[] = {0x00, 0xFF, 0x55, 0x11};
    printf("%04x expected: B8\n", CRC(msg4, 4));

    unsigned char msg5[] = {0x33, 0x22, 0x55, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    printf("%04x expected: CB\n", CRC(msg5, 9));
    return 0;
}