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

# Disable dithering?
if 0:
	dev.write(1, '\x80\x01')

# Set up a default color LUT

lut = [0] * (64 * 25)
for index in range(25):
	lut[index*64] = index | 0x40
lut[24*64] |= 0x20
for channel in range(3):
	for row in range(256):
		value = int(pow(row / 256.0, 2.2) * 0x10000)
		i = (channel << 8) + row
		packetNum = i / 31
		packetIndex = i % 31
		lut[packetNum*64 + 2 + packetIndex*2] = value & 0xFF
		lut[packetNum*64 + 3 + packetIndex*2] = value >> 8
dev.write(1, ''.join(map(chr, lut)))
print "LUT programmed"

# Slowly push random frames to the device

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
		#time.sleep(0.1)

	print
	time.sleep(2)

