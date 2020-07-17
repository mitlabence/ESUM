from arduino import Arduino
from hexfile import HexLine, HexFile
from crc import CRC
from time import sleep
from random import random

def transmit_row(i2c: Arduino, row_address: int, row, dt):
    i2c.set_flash_address(row_address)
    sleep(dt)
    i2c.erase_from_flash()
    sleep(dt)
    i2c.write_to_buffer(row[:16])
    sleep(dt)
    i2c.write_to_buffer(row[16:32])
    sleep(dt)
    i2c.write_to_buffer(row[32:48])
    sleep(dt)
    i2c.write_to_buffer(row[48:])
    sleep(dt)
    crc_value = int(i2c.buffer_to_flash().decode().rstrip(), 16)
    sleep(2*dt)
    return crc_value

def make_noisy(data, p = 0.2):
    noisy_data = data
    for i in range(8):
        if random() < p:
            noisy_data ^= 1 << i
    return noisy_data

def noisy_transmit_row(i2c: Arduino, row_address: int, row_data, dt):
    row = list(row_data)
    for i in range(len(row)):
        if random() > 0.999: #0.985 gives a fair amount of noise
            row[i] = make_noisy(row[i])
    i2c.set_flash_address(row_address)
    sleep(dt)
    i2c.erase_from_flash()
    sleep(dt)
    i2c.write_to_buffer(row[:16])
    sleep(dt)
    i2c.write_to_buffer(row[16:32])
    sleep(dt)
    i2c.write_to_buffer(row[32:48])
    sleep(dt)
    i2c.write_to_buffer(row[48:])
    sleep(dt)
    crc_value = int(i2c.buffer_to_flash().decode().rstrip(), 16)
    sleep(2*dt)
    return crc_value

def erase_all(i2c, dt = 0.005):
    flash_address = 0x1000
    while flash_address < 0x1FFF:
        i2c.set_flash_address(flash_address)
        sleep(dt)
        i2c.erase_from_flash()
        flash_address = flash_address + 32 #1 row consists of 32 words.

def main():
    """
    Don't forget to check the appropriate COM port (i.e., if the Arduino is on COM3, make sure COM_PORT in arduino.py is "COM3")
    """
    crc = CRC()
    dt = 0.005 #Still experimenting with waiting time after each step.
    i2c = Arduino(0x1f) #as of june 2020, bootloader i2c address is the same for all stages
    #hfile = HexFile("firmware_v7.hex") #stage 1
    hfile = HexFile("level2_firmware.hex") #stages 2 and 3

    hfile.import_hex()
    hfile.format_hex_dict()
    for key in hfile.hex_dict.keys(): #since python 3.7, this assures iterating through the keys (flash mamory addresses) in insertion order. Not that it matters much, anyway.
        row_address = int(key, 16)
        row = hfile.hex_dict[key]
        #print(hex(row_address))
        #print(list(map(hex,row)))
        #input()
        crc_calculated = crc.CRC(row)
        i = 0
        while i < 10:
            #crc_val = transmit_row(i2c, row_address, row, dt)
            crc_val = noisy_transmit_row(i2c, row_address, row, dt)
            if crc_val != crc_calculated:
                if i < 9:
                    print(f"Line {hex(row_address)} CRC mismatch detected (received {crc_val}, expected {crc_calculated}). Re-submitting. #{i}")
                    i = i + 1
                else:
                    print(f"Line {hex(row_address)} CRC mismatch could not be resolved within 10 attempts. Continuing with next line. WARNING: This means the uploaded firmware will probably not work properly.")
                    break
            else:
                break
        """
        i2c.set_flash_address(row_address)
        sleep(dt)
        i2c.erase_from_flash()
        sleep(dt)
        i2c.write_to_buffer(row[:16])
        sleep(dt)
        i2c.write_to_buffer(row[16:32])
        sleep(dt)
        i2c.write_to_buffer(row[32:48])
        sleep(dt)
        i2c.write_to_buffer(row[48:])
        sleep(dt)
        crc_lis.append(i2c.buffer_to_flash())
        sleep(2*dt)
        """
    sleep(dt)
    i2c.switch_to_program()
main()

"""
TODO other than hexloader:
    3. EnergySum: a cleaned-up version of state-machine.
TODO
    0. IMPLEMENT CODE TO SKIP CONFIG BIT SETTINGS IN HEX! (config bits can only be changed via programmer, PIC itself cannot do it)
    1. Shorten timing as much as possible (both in hexloader.py, and in arduino.py! Read data sheet for necessary waiting times)
    2. Implement a flag in PIC bootloader and firmware
    3. Clean up this file (HexLine and HexFile can be combined, for example)
    4. Implement readout to hex file
"""
