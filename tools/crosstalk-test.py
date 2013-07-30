#!/usr/bin/env python

# Open Pixel Control client: Test crosstalk between LED strips;
# send each strip a different pattern, use a lot of low-brightness
# pixels so that glitches show up clearly.

import opc, time

client = opc.Client('localhost:7890')

while True:
	for strip in range(8):
		pixels = [ (40,40,40) ] * 512
		for i in range(32):
			pixels[strip * 64 + i * 2] = (100,100,100)

		client.put_pixels(pixels)
		time.sleep(0.5)
