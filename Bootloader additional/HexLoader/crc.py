"""
TODO: completely eliminate magic numbers like 8.
CRC-8-SAE J1850
https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf
CRC result width: 8 bits
Polynomial: 1Dh
Initial value: FFh
Input data reflected: No
Result data reflected: No
XOR value: FFh
Check: 4Bh
Magic check: C4h

Check values (last byte is CRC returned):
00 00 00 00 			59
F2 01 83 			37
0F AA 00 55 			79
00 FF 55 11 			B8
33 22 55 AA BB CC DD EE FF 	CB
92 6B 55 			8C
FF FF FF FF 			74
"""
class CRC:
    def __init__(self):
        self.POLYNOMIAL = 0x1D
        self.init_val = 0xFF
        self.INITIAL_REMAINDER = 0xFF
        self.FINAL_XOR_VALUE = 0xFF
        self.crcTable = []
        self.WIDTH = 8 #number of bits
        self.TOPBIT = 1 << (self.WIDTH - 1)
        self.crcTableInit()
    def crcTableInit(self):
        crcTable = [0 for i in range(256)]
        remainder = 0
        for dividend in range(256):
            remainder = dividend << (self.WIDTH - 8)
            bit = 8
            for bit in range(8):
                if (remainder & self.TOPBIT): #remainder starts with 1
                    remainder = ((remainder << 1)%256 ^ self.POLYNOMIAL)%256
                else: #remainder starts with 0
                    remainder = (remainder << 1)%256
            crcTable[dividend] = remainder
        self.crcTable = crcTable
    def CRC(self, message_list):
        data = 0
        remainder = self.INITIAL_REMAINDER
        for i in range(len(message_list)):
            data = (message_list[i] ^ ((remainder >> (self.WIDTH - 8))%256))%256
            remainder = self.crcTable[data] ^ ((remainder << 8)%256)
        return (remainder ^ self.FINAL_XOR_VALUE)%256
