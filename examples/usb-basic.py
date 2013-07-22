#!/usr/bin/env python
#
# Basic example for Fadecandy, talking directly to the
# Teensy using PyUSB.
#
# Micah Elizabeth Scott
# This example code is released into the public domain.
#

import usb.core
import usb.util
import random
import time
import binascii

dev = usb.core.find(idVendor=0x1d50, idProduct=0x607a)
if not dev:
	raise IOError("No Fadecandy interfaces found")

dev.set_configuration()

print "Serial number: %s" % usb.util.get_string(dev, 255, dev.iSerialNumber)


while True:

	for index in range(25):
		if index == 24:
			# Final
			control = index | 0x20
		else:
			control = index

		data = chr(control) + ''.join(chr(random.randrange(256)) for i in range(63))
		dev.write(1, data)
		print binascii.b2a_hex(data)
		time.sleep(0.1)

	print
	time.sleep(2)

