#!/usr/bin/env python
#
# Find a Fadecandy device, and ask it to reboot back to the Teensy bootloader.
#
# Micah Elizabeth Scott
# This example code is released into the public domain.
#

import usb.core

dev = usb.core.find(idVendor=0x1d50, idProduct=0x607a)
if not dev:
    raise IOError("No Fadecandy interfaces found")

dev.set_configuration()
print "Serial number: %s" % usb.util.get_string(dev, 255, dev.iSerialNumber)
dev.ctrl_transfer(0x40, 1)
print "Rebooted."
