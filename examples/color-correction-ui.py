#!/usr/bin/env python
#
# Simple example color correction UI.
# Talks to an fcserver running on localhost.
#
# Micah Elizabeth Scott
# This example code is released into the public domain.
#

import Tkinter as tk
import socket
import json
import struct

s = socket.socket()
s.connect(('localhost', 7890))
print "Connected to OPC server"

def setGlobalColorCorrection(**obj):
    msg = json.dumps(obj)
    s.send(struct.pack(">BBHHH", 0, 0xFF, len(msg) + 4, 0x0001, 0x0001) + msg)

def update(_):
    setGlobalColorCorrection(
        gamma = gamma.get(),
        whitepoint = [
            red.get(),
            green.get(),
            blue.get(),
        ])

def slider(name, from_, to):
    s = tk.Scale(root, label=name, from_=from_, to=to, resolution=0.01,
        showvalue='yes', orient='horizontal', length=400, command=update)
    s.set(1.0)
    s.pack()
    return s

root = tk.Tk()
root.title("Fadecandy Color Correction Example")

gamma = slider("Gamma", 0.2, 3.0)
red = slider("Red", 0.0, 1.5)
green = slider("Green", 0.0, 1.5)
blue = slider("Blue", 0.0, 1.5)

root.mainloop()
