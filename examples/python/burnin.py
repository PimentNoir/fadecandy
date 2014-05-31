#!/usr/bin/env python

# Burn-in test: Keep LEDs at full brightness most of the time, but dim periodically
# so it's clear when there's a problem.

import opc, time, math

numLEDs = 512
client = opc.Client('localhost:7890')

t = 0

while True:
    t += 0.4
    brightness = int(min(1, 1.25 + math.sin(t)) * 255)
    frame = [ (brightness, brightness, brightness) ] * numLEDs
    client.put_pixels(frame)
    time.sleep(0.05) 
