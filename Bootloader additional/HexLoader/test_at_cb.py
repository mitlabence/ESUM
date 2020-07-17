from arduino import Arduino
"""

"""
I2C_ADDRESS = 0x1e
TESTPULSER_FLAG = 0x50
MDAC_FLAG = 0x18
class Test:
    def __init__(self):
        self.a = Arduino(I2C_ADDRESS) #I2C with firmware
    def switchToESUM(self):
        self.a.change_address(0x1f)
        #self.a.close() #free up COM port
        #self.a = Arduino(0x1f) #talk to firmware first
        self.a.switch_to_program()
        self.a.change_address(I2C_ADDRESS)
        #self.a.close()
        #self.a = Arduino(I2C_ADDRESS)
    def setODAC(self, val):
        lower_byte = val%(256)
        higher_byte = (val - lower_byte)//256
        print(f"Setting {hex(lower_byte + higher_byte*256)}.")
        self.a.send_bytes_list([0x21, higher_byte, lower_byte])
        self.a.send_bytes_list([0x20, 1])
    def close(self):
        self.a.close()
    def setAllMDACs(self, value):
        self.a.send_bytes_list([0x00] + [value for i in range(24)] + [0x02])
    def setMDACflag(self, value):
        self.a.send_bytes_list([MDAC_FLAG, value])
    def setMDAC(self, channel, value): #only changes one MDAC (1-24) channel
        self.a.send_bytes_list([channel-1, value])
        self.setMDACflag(0x02) #update all channels
    def testSingleMDAC(self, channel, value): #turn off all MDACs other than one; set this one to a given gain. Channels: 1-24!
        self.setAllMDACs(0x00)
        self.a.send_bytes_list([channel-1, value])
        self.setMDACflag(0x02)
    def testChannel(self): #as long as input is between 1-24, keep testing (chosen channel set to max, others to 0)
        not_quit = True
        while not_quit:
            val = int(input())
            if val > 24:
                not_quit = False
                break
            self.testSingleMDAC(val, 0xff)
            self.readOutValues()
    def readOutValues(self): #read out MDAC gain values to check if working as it should
        self.a.read_n_bytes(0x00, 24)
    def switchTP(self, on = True, val = 0xff): #set channel 24 to val, turn on TP if on = True, off if on = False
        self.testSingleMDAC(24, val)
        if on:
            s = 0x01 #turn TP on if on is true
        else:
            s = 0x00
        self.a.send_bytes_list([TESTPULSER_FLAG, s]) #switch TP on
    def readUID(self):
        #self.a.close()
        #self.a = Arduino(0x51) #open i2c with UID chip
        self.a.change_address(0x51)
        self.a.read_n_bytes(0xfa, 6) #read out full address of 6 bytes. Last 3 are written on ESUM boards.
        #self.a.close()
        #self.a = Arduino(I2C_ADDRESS)
        self.a.change_address(I2C_ADDRESS)
    def setBL(self):
        self.a.send_bytes_list([0x51, 0x01]) #Baseline flag is 0x51, set bit 0 to 1 to initiate BL-setting; is cleared automatically.
"""
t = Test()
t.testChannel()
"""
