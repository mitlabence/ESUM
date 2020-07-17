from test_at_cb import Test
from time import sleep

t = Test()
t.switchToESUM()
sleep(0.5)
t.switchTP()
sleep(4)
t.switchTP(False)
t.close()
