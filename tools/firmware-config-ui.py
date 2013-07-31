#!/usr/bin/env python
#
# Simple UI for firmware configuration flags.
# Talks to an fcserver running on localhost.
#
# Micah Elizabeth Scott
# This example code is released into the public domain.
#

import Tkinter as tk
import socket
import struct

s = socket.socket()
s.connect(('localhost', 7890))
print "Connected to OPC server"

def setFirmwareConfig(data):
    s.send(struct.pack(">BBHHH", 0, 0xFF, len(data) + 4, 0x0001, 0x0002) + data)

def update():
    setFirmwareConfig(chr(
        noDither.var.get() |
        (noInterp.var.get() << 1) |
        (manualLED.var.get() << 2) |
        (ledOnOff.var.get() << 3) ))

def check(text):
    v = tk.IntVar()
    w = tk.Checkbutton(root, command=update, variable=v, text=text)
    w.var = v
    w.pack()
    return w

root = tk.Tk()
root.title("Fadecandy Firmware Configuration UI")

noDither = check("Disable dithering")
noInterp = check("Disable interpolation")
manualLED = check("Built-in LED under manual control")
ledOnOff = check("Built-in LED manual on/off")

root.mainloop()
