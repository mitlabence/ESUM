from time import sleep
import serial

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


Convert from hex file into I2C command:

1. read line
2. check TT for type of command

Needed commands:
    *send command byte: slave address W, send command byte, stop
    *set flash address: slave address W, send command byte, send 2 bytes, stop
    *read flash address: slave address W, send command byte, stop, slave address R, read out LSByte, read out MSByte.
    *write to flash buffer: slave address W, send command byte, send 16 bytes, stop
    *read from flash memory after sending command byte: slave address R, read 16 bytes; MSByte is returned first, then LSByte.
"""

"""
These values should be in accordance with bootloader defined commands.
Before trying to send a hex file, check if they are indeed correct.
>>>>>>>>
"""
CMD_SET_FLASH_ADDRESS = 0x01
CMD_WRITE_TO_BUFFER = 0x02
CMD_READ_FROM_FLASH = 0x03
CMD_ERASE_ROW_FROM_FLASH = 0x04
CMD_BUFFER_TO_FLASH = 0x05
CMD_RUN_APPLICATION = 0x06
CMD_READ_FROM_FLASH_SECOND_HALF = 0x07
CMD_RESET = 0x08

COM_PORT = 'COM3'

I2C_SLAVE_ADDRESS = 0x1f #for first-level ESUM, current convention
BUFFER_SIZE = 64
"""
<<<<<<<<
"""

class Arduino:
    def __init__(self, address: int = 0x1f):
        # define Arduino COM port and speed
        self.arduinoCom = COM_PORT
        self.arduinoSpeed = 115200
        self.address = address
        self.arduino = serial.Serial(self.arduinoCom, self.arduinoSpeed, timeout=.1)
        sleep(2)
    def change_address(self, new_address):
        self.address = new_address
    def send_two_bytes(self, byte1, byte2): #helpful to write to uploaded firmware (set flags, etc.)
        command = f"I2CW 2 {hex(self.address)} {hex(byte1)} {hex(byte2)} \r"
        #print(command)
        self.arduino.write(command.encode())
    def send_bytes_list(self, bytes_list): #send all bytes in bytes_list from [0] to [-1]
        if(len(bytes_list) == 0):
            print("Attempted to send empty list.")
            return 0
        command = f"I2CW {len(bytes_list)} {hex(self.address)} " + " ".join(list(map(hex, bytes_list))) + " \r"
        self.arduino.write(command.encode())
    def read_single_byte(self, position): #in firmware, go to given position, and read from it
        self.send_command_byte(position) #send single byte
        return self.read_byte()
    def read_n_bytes(self, position, n): #in firmware, read n bytes starting from position
        self.send_command_byte(position) #send single byte
        command = f"I2CR {n} {hex(self.address)} \r"
        self.arduino.write(command.encode())
        for i in range(n):
            readback = self.arduino.readline()
            print(f"{i+1}: " + readback.decode().strip())
    def send_command_byte(self, cmd_byte):
        command = f"I2CW 1 {hex(self.address)} {hex(cmd_byte)} \r"
        #print(command)
        self.arduino.write(command.encode())
    def read_byte(self):
        command = f"I2CR 1 {hex(self.address)} \r"
        self.arduino.write(command.encode())
        readback = self.arduino.readline()
        return readback
    def set_flash_address(self, address): #bootloader takes LSByte first, then MSByte.
        #NOTE: address is words address; in hex file, bytes address = words address * 2 is given!
        LSByte = address % 256
        MSByte = int((address - LSByte) / 256)
        command = f"I2CW 3 {hex(self.address)} {hex(CMD_SET_FLASH_ADDRESS)} {hex(LSByte)} {hex(MSByte)} \r"
        self.arduino.write(command.encode())
    def write_to_buffer(self, bytes_list): #bytes_list should be list of int, always LSByte, then MSByte ([LSByte0, MSByte0, LSByte1, MSByte1, ..., LSByte15, MSByte15])
        #take 16 bytes, i.e. 8 words, write them to buffer
        command = f"I2CW {len(bytes_list) + 1} {hex(self.address)} {hex(CMD_WRITE_TO_BUFFER)} "
        for byte in bytes_list:
            command = command + f"{hex(byte)} "
        command = command + "\r"
        #print(command)
        self.arduino.write(command.encode())
    """
    def read_from_flash_memory(self): #returns list of int of form [MSB0, LSB0, MSB1, LSB1, ...]
        self.send_command_byte(CMD_READ_FROM_FLASH)
        command = f"I2CR {BUFFER_SIZE} {hex(self.address)} \r"
        self.arduino.write(command.encode())
        readback = self.arduino.read(256)
        #readback = self.arduino.read(256).decode().split("\r\n") #first decode the b"" byte-string, then split into bytes. 256 bytes should be enough.
        #return list(map(int, readback))
        return readback
    """
    def read_first_half(self):
        self.send_command_byte(CMD_READ_FROM_FLASH)
        command = f"I2CR {BUFFER_SIZE} {hex(self.address)} \r"
        self.arduino.write(command.encode())
        readback = self.arduino.read(256).decode().rstrip().split("\r\n")
        #readback = self.arduino.read(256).decode().split("\r\n") #first decode the b"" byte-string, then split into bytes. 256 bytes should be enough.
        #return list(map(int, readback))
        return [byte.rjust(2, "0") for byte in readback] #e.g. "0" -> "00", "A" -> "0A" etc.
    def read_second_half(self):
        self.send_command_byte(CMD_READ_FROM_FLASH_SECOND_HALF)
        command = f"I2CR {BUFFER_SIZE} {hex(self.address)} \r"
        self.arduino.write(command.encode())
        readback = self.arduino.read(256).decode().rstrip().split("\r\n")
        return [byte.rjust(2, "0") for byte in readback] #e.g. "0" -> "00", "A" -> "0A" etc.
        #readback = self.arduino.read(256).decode().split("\r\n") #first decode the b"" byte-string, then split into bytes. 256 bytes should be enough.
        #return list(map(int, readback))
    def join_words(self, bytes_list):
        words_list = []
        i = 0
        while i < (len(bytes_list) - 1):
            words_list.append(bytes_list[i] + bytes_list[i + 1])
            i = i + 2
        return words_list
    def read_from_flash_memory(self):
        l = self.join_words(self.read_first_half())
        print(" ".join(l[:int(len(l)/2)]))
        print(" ".join(l[int(len(l)/2):]))
        l = self.join_words(self.read_second_half())
        print(" ".join(l[:int(len(l)/2)]))
        print(" ".join(l[int(len(l)/2):]))
    def buffer_to_flash(self):
        self.send_command_byte(CMD_BUFFER_TO_FLASH)
        sleep(0.005)
        return self.read_byte()
    def erase_from_flash(self):
        self.send_command_byte(CMD_ERASE_ROW_FROM_FLASH)
        sleep(0.005)
        return self.read_byte()
    def switch_to_program(self):
        self.send_command_byte(CMD_RUN_APPLICATION)
        return self.read_byte()
    def turn_on_firmware_leds(self, fw_addr):
        #set 0x52 to 0x01
        command = f"I2CW 2 {hex(fw_addr)} 0x52 0x01 \r"
        #print(command)
        self.arduino.write(command.encode())
    def erase_all(self, dt = 0.005):
        flash_address = 0x1000
        while flash_address < 0x1FFF:
            self.set_flash_address(flash_address)
            sleep(dt)
            self.erase_from_flash()
            flash_address = flash_address + 32 #1 row consists of 32 words.
    def reset(self):
        self.send_command_byte(CMD_RESET)
        sleep(0.005)
        return self.read_byte()
    def close(self):
        self.arduino.close()
