/*
 * AD5429
 * 8-bit multiplying DAC
 * 
 * Timing characteristics:  5/28
 * Command structure:       20/28
 * 
 * 
 * Quickly get MDAC working:
 * 1. Initialize SPI:   
    SPI_Init(1,0);
 * 2. Initialize MDAC:  
    Set_CS(1);
    MDAC_Init(0, 0, 0);
    Set_CS(2);
 * 3. Communicate:      
    Set_CS(1);
    MDAC_LoadUpdateSingle(), MDAC_ReadSingle(), MDAC_LoadUpdateAll(), MDAC_ReadAll();
    Set_CS(2);
 */


#include <xc.h>
#include "SPI.h"
#include "MDAC.h"
#include "SPI.h" //Needed for Set_CS()

extern unsigned char memory[256]; //memory[] from main.c

/* Converts given control and data bits into single message (2-byte array)
 * 
 * The array will look like
 * [C3 C2 C1 C0 D7 D6 D5 D4,
 *  D3 D2 D1 D0  0  0  0  0]
 * 
 * This function overwrites the passed array values, preparing an array containing 
 * the wanted control bits and data bits, compatible with the SPI library.
 * Control bits: AD5429 data sheet p. 21/28
    0 0 0 0 A and B No operation (power-on default)
    0 0 0 1 A       Load and update
    0 0 1 0 A       Initiate readback
    0 0 1 1 A       Load input register
    0 1 0 0 B       Load and update
    0 1 0 1 B       Initiate readback
    0 1 1 0 B       Load input register
    0 1 1 1 A and B Update DAC outputs
    1 0 0 0 A and B Load input registers
    1 0 0 1 N/A     Disable daisy-chain
    1 0 1 0 N/A     Clock data to shift register on rising edge
    1 0 1 1 N/A     Clear DAC output to zero scale
    1 1 0 0 N/A     Clear DAC output to midscale
    1 1 0 1 N/A     Control word
    1 1 1 0 N/A     Reserved
    1 1 1 1 N/A     No operation 
 */
void MDAC_CommandToArray(unsigned char control_bits, unsigned char data_bits, unsigned char *mdac_array){
    mdac_array[0] = 0;
    mdac_array[1] = 0;
    mdac_array[0] |= control_bits;      // 0 0 0 0 C3 C2 C1 C0
    mdac_array[0] <<= 4;
    mdac_array[1] |= data_bits;         // D7 ...           D0
    mdac_array[1] >>= 4;                // 0 0 0 0 D7 D6 D5 D4
    mdac_array[0] |= mdac_array[1];     // C3 C2 C1 C0 D7 D6 D5 D4
    mdac_array[1] = 0;                  // clear value
    mdac_array[1] |= data_bits;         // D7 ...           D0
    mdac_array[1] <<= 4;                // D3 D2 D1 D0 0 0 0 0
    /*
    //alternative solution, both seem to work
    
    unsigned char element0 = 0;
    unsigned char element1 = 0;
    element0 |= control_bits;
    element0 <<= 4;
    element1 |= data_bits;
    element1 >>= 4;
    element0 |= element1;
    element1 = 0;
    element1 |= data_bits;
    element1 <<= 4;
    *(mdac_array) = element0;
    *(mdac_array + 1) = element1;
    */
}


/*
 * NEED TO SET CHIP SELECT EXTERNALLY (Set_CS in SPI.c)!
 * control_bits: see MDAC_Command description for table
 * Timing diagrams: AD5429 p. 6/28
 */
unsigned char MDAC_IO_Single(unsigned char control_bits, unsigned char data_bits){
    unsigned char answer;
    unsigned char helpful_var;
    unsigned char MDAC_array[MDAC_bytes_number];
    MDAC_array[0] = 0;
    MDAC_array[1] = 0;
    answer = 0;
    helpful_var = 0;
    MDAC_CommandToArray(control_bits, data_bits, MDAC_array);
    SPI_IO(MDAC_array, MDAC_bytes_number);
    //Get 8-bit answer data from 16-bit MISO signal
    answer |= MDAC_array[0];        //C3 ... C0 D7   ... D4
    helpful_var |= MDAC_array[1];   //D3 ... D0 0  0  0  0
    answer <<= 4;                   //D7 ... D4 0  0  0  0
    helpful_var >>= 4;              // 0 0 0 0  D3 D2 D1 D0
    answer |= helpful_var;      
    return answer;
}

/*
 * Get number of MDAC (1-12) that contains given index (1-24). WARNING: channel 24 is testpulser!
 * E.g. MDAC 2 corresponds to chip 1 (channel B)
 */
unsigned char MDAC_GetNumber(unsigned char index){
     unsigned char MDAC_number;  //converts 1-23 index to 1-12 MDAC number, see circuit
     /*  
     *  To get MDAC number (1-12) from ESUM_channel (1-24):
     *      even channel: integer-divide by 2
     *       odd channel: increment then integer-divide by 2
     */
    MDAC_number = index;
    MDAC_number++;      //if even channel, incremented LSB disappears in next step anyway
    MDAC_number >>= 1;
    //In daisy-chain, the order is reversed, that is, index = 1 corresponds to the first MDAC, which gets the last message out of the 12 or 23.
    MDAC_number = daisy_chain_length + 1 - MDAC_number;
    return MDAC_number;
}

/*
 * Checks if given ESUM_channel (1-24) is channel A or B of an MDAC.
 * If channel is even, it is B.
 */
unsigned char MDAC_GetChannel_AB(unsigned char index){
    unsigned char is_channel_A;
    //get LSB of ESUM_channel to check for parity
    is_channel_A = index;
    is_channel_A <<= 7;
    is_channel_A >>= 7;
    //even channels are B, odd are A (CH1 = B1 IN_A, CH2 = B1 IN_B, ...)
    return is_channel_A;
}

/*
 * Update all (daisy_chain_length*2 - 1) = 23 MDACs. Leaves the array unchanged.
 * 
 * data should be an array of 24 entries with data[i] referring to data written to the (i-1)-th channel (1-23, 24 is testpulser, not used): 
 * [MDAC 1 Channel A, MDAC 1 Channel B, MDAC 2 Channel A, ..., MDAC 12 Channel A, arbitrary(MDAC 12 Channel B is testpulser)]
 * That is, data[0] will be sent to MDAC channel 1, data[22] to MDAC channel 23, data[23] ignored.
 * 
 * IMPORTANT: data[0] is written last to ensure that the recipient is MDAC 1 channel A (in circuit diagram).
 * TODO: needs to be tested!!!
 * TODO: remove CS from MDAC.c!!! Perhaps write function MDAC_LoadUpdateAll(channel (A or B), unsigned char *data), and implement function below in common.c!
 */


    
    //MDAC_IO_Single(0, 0);
    /*
    for(i = (2*daisy_chain_length - 2); i > 1; i--){
        MDAC_IO_Single(MDAC_WriteA, data[i]);
        i--;
        MDAC_IO_Single(MDAC_WriteB, data[i]);
    }
    MDAC_IO_Single(MDAC_WriteA, data[0]);
     */


/*
 * Level 1 ESUM:
 *  Can be used for testpulser (index = 24)
 *  index should be in range [1, 24]
 * Level 2 ESUM:
 *  Has 4 AD5429 chips -> 8 channels, daisy_chain_length = 4 should be set
 */
void MDAC_LoadUpdateSingle(unsigned char index, unsigned char data){
    unsigned char MDAC_number;  //1-12
    unsigned char MDAC_channel; //0 if channel B, 1 if A of given MDAC
    unsigned char control_bits; //write to A or write to B
    unsigned char i;            //for loop variable
    MDAC_number = MDAC_GetNumber(index);
    MDAC_channel = MDAC_GetChannel_AB(index);
    
    if(MDAC_channel == 0){
        control_bits = MDAC_WriteB; 
    }
    else{
        control_bits = MDAC_WriteA;
    }
    
    //NOP to channels 
    for(i = 1; i < MDAC_number; i++){
        MDAC_IO_Single(MDAC_NOP, 0);
    }
    MDAC_IO_Single(control_bits, data);
    i++;
    for(; i <= daisy_chain_length; i++){
        MDAC_IO_Single(MDAC_NOP, 0);
    }
}
unsigned char MDAC_ReadSingle(unsigned char index){
    unsigned char data;
    unsigned char MDAC_number;  //1-12
    unsigned char MDAC_channel; //0 if channel B, 1 if A of given MDAC
    unsigned char control_bits; //write to A or write to B
    unsigned char i;            //for loop variable
    MDAC_number = MDAC_GetNumber(index);
    MDAC_channel = MDAC_GetChannel_AB(index);
    
    if(MDAC_channel == 0){
        control_bits = MDAC_ReadB; 
    }
    else{
        control_bits = MDAC_ReadA;
    }
    
    //NOP to channels 
    for(i = 1; i < MDAC_number; i++){
        MDAC_IO_Single(MDAC_NOP, 0);
    }
    
    MDAC_IO_Single(control_bits, 0);
    i++;
    
    for(; i <= daisy_chain_length; i++){
        MDAC_IO_Single(MDAC_NOP, 0);
    }
    
    //The readback data is going to appear this time
    //NOP to channels 
    for(i = 1; i < MDAC_number; i++){
        MDAC_IO_Single(MDAC_NOP, 0);
    }
    
    data = MDAC_IO_Single(MDAC_NOP, 0);
    i++;
    
    for(; i <= daisy_chain_length; i++){
        MDAC_IO_Single(MDAC_NOP, 0);
    }
    
    return data;
}

/*
 * see pp. 20-22/28
 * SDO_control: 
 *      0 if full SDO driver,       (default)
 *      1 if weak SDO driver,
 *      2 if SDO configured as open drain,
 *      3 if disable SDO output
 * clear_to_midscale:
 *      0 if on reset the DAC outputs should be cleared to zero,    (default)
 *      1 if on reset (CLR pin) the DAC outputs should be cleared to midscale
 * SCLK (active clock edge):
 *      0 if active clock edge should be falling edge,              (default)
 *      1 if active clock edge should be rising edge
 * (Cannot change with this function: DSY bit (enable daisy chain, always enable))
 * 
 * ODAC by default has rising edge as active clock edge, so SCLK=1 makes MDACs compatible with ODAC
 */
void MDAC_Init(unsigned char SDO_control, unsigned char clear_to_midscale, unsigned char SCLK){
    unsigned char data;
    data = 0;
    data |= SDO_control;    // 0 0 0 0 0 0 SDO2 SDO1
    data <<= 1;
    data |= 1;              //DSY bit, 1 is enable, 0 is disable; we never turn off daisy chain, so no need to use parameter
    data <<= 1;
    data |= clear_to_midscale;
    data <<= 1;
    data |= SCLK;
    data <<= 3;             //SDO2 SDO1 DSY HCLR SCLK 0 0 0
    //send same command to the MDAC chips
    unsigned char i;
    for(i = 0; i < daisy_chain_length; i++){
        MDAC_IO_Single(MDAC_ControlWord, data);
    }
}
/*
 * Updates the 24 MDACs to memory[first_index] - memory[first_index+23] values
 */
void MDAC_LoadUpdate(unsigned char first_index){
    unsigned char i;
    //loop through the 12 MDAC units: first, write to channel B of each, then to channel A
    Set_CS(1);
    for(i = 0; i < 2*daisy_chain_length-1; i+=2){//write to MDAC channels 24, 22, 20, ..., 2; these correspond to (MDAC_1 + 23) - i (i = 0, 2, 4, ..., 22)
        MDAC_IO_Single(MDAC_WriteB, memory[first_index + 2*daisy_chain_length - 1 - i]);
    }
    
    Set_CS(2);
    Set_CS(1);
    
    //MDAC channels 23, 21, ..., 1
    for(i = 1; i < 2*daisy_chain_length; i+=2){//write to DMAC channels 23, 21, ..., 1; these correspond to (MDAC_1 + 23) - i (i = 1, 3, ..., 23)
        MDAC_IO_Single(MDAC_WriteA, memory[first_index + 2*daisy_chain_length - 1 - i]);
    }
    
    Set_CS(2);
}

/*
 * Reads the 24 MDAC values and writes them into memory[MDAC_1] - memory[MDAC_1 + 23]
 * TODO: Not tested yet!
 */
void MDAC_Read(void){
    unsigned char i;
    //Loop through B channel (even channels, e.g. 24, 22, ..., 2 for ESUM level 1), then A (23, 21, ..., 1)
    Set_CS(1);
    
    //Loop through channel B first; this time, no output is present
    for(i = 0; i < daisy_chain_length; i++){//0, 1, ..., 23
        MDAC_IO_Single(MDAC_ReadB, 0);
    }
    
    Set_CS(2);
    Set_CS(1);
    
    //Loop through channel A; this time, data from previous loop is received
    for(i = 0; i < 2*daisy_chain_length-1; i+=2){
        memory[MDAC_1 + (2*daisy_chain_length - 1) - i] = MDAC_IO_Single(MDAC_ReadA, 0); //Last chip will be read out first (for example, channel 24, saved to memory[23])
    }
    
    Set_CS(2);
    Set_CS(1);
    
    //data from commands sent in previous loop is received
    for(i = 1; i < 2*daisy_chain_length; i+=2){
        memory[MDAC_1 + (2*daisy_chain_length - 1) - i] = MDAC_IO_Single(MDAC_NOP, 0); //start with memory[22], for example, for channel 23
    }
    Set_CS(2);
}

/* 
 * send same command [*4 control bits*, 8 data bits] to all MDACs
 * CS already inside function!
 */
void MDAC_SendToAll(unsigned char control_bits, unsigned char data_byte){
    unsigned char i;
    //loop through the 12 MDAC units: first, write to channel B of each, then to channel A
    Set_CS(1);
    for(i = 0; i < daisy_chain_length; i++){//write to MDAC channels 24, 22, 20, ..., 2; these correspond to (MDAC_1 + 23) - i (i = 0, 2, 4, ..., 22)
        MDAC_IO_Single(control_bits, data_byte);
    }
    Set_CS(2);
}

/*
 * Used solely to turn SDO of last MDAC on and off 
 */
void MDAC_SendToLast(unsigned char control_bits, unsigned char data_byte){
    unsigned char i;            //for loop variable
    Set_CS(1);
    MDAC_IO_Single(control_bits, data_byte); //first command sent reaches last MDAC
    for(i = 0; i < (daisy_chain_length-1); i++){
        MDAC_IO_Single(MDAC_NOP, 0);
    }
    Set_CS(2);
}