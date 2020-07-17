from test_at_cb import Test
from arduino import Arduino
from time import sleep
def testOutput():
    a = Arduino(0x1f)
    a.switch_to_program()
    sleep(0.05)
    a.close()
    t = Test()
    t.switchTP()
    sleep(3)
    t.switchTP(False)
    t.close()

while (input() != "q"):
    testOutput()
