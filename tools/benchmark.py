#!/usr/bin/env python
#
# Read the device's performance counters, calculate benchmark values.
#
# Micah Elizabeth Scott
# This example code is released into the public domain.
#

import usb.core
import usb.util
import time, struct, sys

dev = usb.core.find(idVendor=0x1d50, idProduct=0x607a)
if not dev:
    raise IOError("No Fadecandy interfaces found")

dev.set_configuration()

print "Serial number: %s" % usb.util.get_string(dev, 255, dev.iSerialNumber)

def readFrameCounter():
    return struct.unpack('<I', dev.ctrl_transfer(0xC0, 0x01, 0, 0, 4, 1000))[0]

sys.stderr.write("Calculating frame rate average...\n")
t1 = time.time()
c1 = readFrameCounter()

try:
    while True:
        time.sleep(0.1)
        t2 = time.time()
        c2 = readFrameCounter()
        fps = ((c2 - c1) & 0xFFFFFFFF) / (t2 - t1)
        sys.stderr.write("\r   %.3f FPS   " % fps)
except KeyboardInterrupt:
    sys.stderr.write("\n")

