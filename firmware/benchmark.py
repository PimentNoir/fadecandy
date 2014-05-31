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

def readCounter(index):
    return struct.unpack('<I', dev.ctrl_transfer(0xC0, 0x01, 0, index, 4, 1000))[0]

def counterDiff(c1, c2):
    return (c2 - c1) & 0xFFFFFFFF

def initLUT():
    # Set up a default color LUT
    lut = [0] * (64 * 25)
    for index in range(25):
        lut[index*64] = index | 0x40
    lut[24*64] |= 0x20
    for channel in range(3):
        for row in range(257):
            value = min(0xFFFF, int((row / 256.0) * 0x10000))
            i = channel * 257 + row
            packetNum = i / 31
            packetIndex = i % 31
            lut[packetNum*64 + 2 + packetIndex*2] = value & 0xFF
            lut[packetNum*64 + 3 + packetIndex*2] = value >> 8
    lutPackets = ''.join(map(chr, lut))
    dev.write(1, lutPackets)
    print "LUT programmed"

def dummyFrame(byte):
    buffer = ''
    for i in range(25):
        final = i == 24
        buffer += chr( (final << 5) | i ) + chr(byte) * 63
    return buffer

def measureFrameRate():
    sys.stderr.write("Calculating frame rate average...\n")
    duration = 8.0
    t1 = t2 = time.time()
    c1 = readCounter(0)     # Rendered frames
    k1 = readCounter(1)     # Received keyframes

    # Dummy frames
    data = (dummyFrame(0) + dummyFrame(0x80)) * 200

    # Use number of keyframes handled per second to estimate USB bandwidth
    megabitsPerFrame = len(dummyFrame(0)) * (8.0 / 1e6)

    # Average the frame rate for up to 'duration' seconds.
    try:
        while (t2 - t1) < duration:
            # Keep the EP1 OUT pipe full-ish, so we measure it while the USB driver is busy
            dev.write(1, data)

            t2 = time.time()
            c2 = readCounter(0)
            k2 = readCounter(1)

            fps = counterDiff(c1, c2) / (t2 - t1)
            kfps = counterDiff(k1, k2) / (t2 - t1)
            mbps = kfps * megabitsPerFrame

            # Not sure how accurate this USB throughput is, the blocking write() is likely a problem.
            sys.stderr.write("\r   %.2f FPS, %.2f KFPS, %.2f Mbps  " % (fps, kfps, mbps))

    except KeyboardInterrupt:
        pass

    sys.stderr.write("\n")
    return fps

initLUT()
measureFrameRate()
