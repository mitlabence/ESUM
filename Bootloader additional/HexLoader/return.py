from arduino import Arduino
a = Arduino(0x1e)
a.send_two_bytes(0xbb, 0x01)
