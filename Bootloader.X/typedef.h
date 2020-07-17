
#ifndef TYPEDEF_HEADER
#define	TYPEDEF_HEADER

#include <xc.h> // include processor files - each processor file is guarded.  

typedef    struct
    {
        unsigned char byte0;
        unsigned char byte1;
    }int16;
typedef union _ADDRESS
{
	struct 
	{
		int address;
	}word;
	struct
	{
        unsigned char LSbyte; //byte with least significant bits
        unsigned char MSbyte; //byte with most significant bits (i.e. digits on the left)   
 	}bytes;
}  ADDRESS; 

#endif	

