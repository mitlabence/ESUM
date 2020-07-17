"""
Read an Intel hex file, and, converting it into PIC-compatible code on the go, send it via I2C.

Intel hex file structure:

Each line has the form
:BBAAAATT[DDDDDDDD]CC

:       - start of line marker
BB      - number of data BYTES on this line
AAAA    - first address in bytes
TT      - type:
            00 - data
            01 - EOF
            02 - linear address (marking of 64kb, 128 kb, etc., for extended addressing capabilities, as with AAAA only 64kb data could be addressed)
            04 - extended address
DD      - data bytes, amount determined by BB
CC      - checksum (2s-complement of number of bytes + address + data)
"""
class HexLine():
    def __init__(self, line: str): #make sure line is a whole, valid line from a hex file, i.e. starting with ":", etc.
        l = line[1:] #get rid of initial ':'
        BB = l[0:2]
        self.number_bytes = int(BB, 16) #number of bytes self.data consists of
        AAAA = l[2:6]
        TT = l[6:8]
        CC = l[8+2*self.number_bytes:].rstrip()
        DDDD = l[8:8+2*self.number_bytes]
        self.address = int(AAAA, 16)
        self.type = int(TT) #0 for data, 1 for EOF, 2 for linear address, 4 for extended address
        self.checksum = int(CC, 16)
        self.data = [int(DDDD[i:i+2], 16) for i in range(0, len(DDDD), 2)] #convert "XY" = "0xXY" into decimal int representation
    def calculate_checksum(self):
        """
        sum up all bytes in the line, take least significant byte, and take twos complement
        """
        s = (self.number_bytes + self.type + int((self.address - self.address % 256)/256) + self.address % 256 + sum(self.data))%256
        s = ((255 - s) + 1)%256 #invert and add one to form twos complement
        return s

"""
https://www.kanda.com/blog/microcontrollers/pic-microcontrollers/pic-hex-file-format/
IMPORTANT: when exporting hex file, in MPLAB X IDE, Set Project Configuration -> Customize -> XC8 Linker -> Runtime -> make sure Format hex file for download is checked! The hex file should have 0x0000, 0x0010, 0x0020, ... as rows, always a full, 16 byte row!
:020000040000FA means nothing, so skip this line (extended address stuff)
:020000040001F9 means 64KB marker -> next lines have 0x10000 added to address.
Common line: :08000E008CFFF3FFFFDFFFFF91 after 64kb marker. In this case, word address is 0x1000E/2 = 0x8007. This is the configuration word 1 (63/491 in data sheet)
:00000001FF means end of file
"""

class HexFile():
    def __init__(self, fname):
        self.hex_dict = {}
        self.fname = fname
    def get_row_index(self, ind: int): #One row in PIC contains 32 words -> first row is 0x0000, second 0x0020, etc. This function converts an arbitrary index to its row index.
        return (ind - ind%32)
    def print_hex(self):
            for k in self.hex_dict.keys():
                print(f"{k}: {' '.join(list(map(hex, self.hex_dict[k]))) }")
                print(f"{k}: {len(self.hex_dict[k])}")
    def import_hex(self):
        with open(self.fname, "r") as f:
            for line in f:
                hline = HexLine(line)
                #print(f": {hline.number_bytes} {hline.address} {hline.type} {hline.data} {hline.checksum}") #print imported hex file line
                if hline.type == 4:
                    if hline.checksum != int(0xFA): #see extended addressing; if data is not 0, then for the PIC16F18345, we can be sure that it is the config bytes, which are not supported yet in this hexloader.
                        print("Overwriting config bytes not supported by this version of HexLoader.")
                        print(hline.checksum)
                        print(type(hline.checksum))
                        break
                    continue
                elif line[0] != ":":
                    print("Warning: line does not start with ':'!")
                    print(line)
                    print("Skipping this line.\n")
                    continue
                elif line[:9] == ":00000001": #:00000001FF means end of file, do not compare whole line because of possible hidden characters (I am lazy, you know)
                    print("End line reached.")
                    break

                """
                What to do:
                for each line in hex file:
                    1. get PIC address (word address from byte address)
                    2. find PIC row and
                        a) if PIC row already exists, then do a 0x3fff padding, then append current line.
                        b) if PIC row does not exist, create the row, then do a 0x3fff padding, then append current line.
                """
                word_address = int(hline.address/2)
                row_index = self.get_row_index(word_address) #word address of row in PIC to which current hex line belongs to
                lis = []
                if str(hex(row_index)) in self.hex_dict.keys(): #PIC flash memory row already created
                    lis = self.hex_dict[str(hex(row_index))] #copy row content
                    current_index = row_index + len(lis) - 1 #points to word to write to
                else: #row is empty; do 0x3fff padding before copying from hex file line
                    lis = []
                    current_index = row_index
                while current_index < word_address: #padding with 0x3fff
                    lis.append(0xff)
                    lis.append(0x3f)
                    current_index = current_index + 1
                for byte in hline.data:
                    lis.append(byte)
                self.hex_dict[str(hex(row_index))] = lis
    def format_hex_dict(self):
        for k in self.hex_dict.keys():
            lis = self.hex_dict.get(k)
            while len(lis) < 64:
                if len(lis)%2:#odd length, for example 1: then 0xff should be only element. -> Append 0x3f. (0x3fff is the "default" word in PIC)
                    lis.append(0x3f)
                else:
                    lis.append(0xff)
            self.hex_dict[k] = lis
